/* DQK.EXE resident module 19a4: tile libraries, palettes and the screen
 * flush.  Opening a .tlb tile library into a slot, the resource handles
 * that name a slot and an index in it, glyph drawing through those
 * handles, the palette buffer and its upload, and the per-plane row
 * buffers.
 *
 * EXE 0x00be49..0x00ca24, one Turbo C large-model _TEXT segment starting at
 * byte 9 of paragraph 19a4.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee and variable names are placeholders chosen from use; a
 * modNNNN_ name says only which segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is: glyph_cell
 * and glyph_at far-call screen_draw_begin, and screen_flush_mode_set far-calls
 * screen_flush, so those two carry prototypes.
 *
 * _BSS follows the statics' names (Turbo C's symbol-table order), not their
 * declaration order; these names reproduce the shipped a2ae..a629 layout,
 * chosen with goldbox.bss_order.  The originals are not recoverable from it.
 * rows_saved's [5] is what fits between a319 and resources at a5e9; no code
 * here bounds the plane count.
 *
 * 0x454c4954L is the tag "TILE" as four bytes.  Cell coordinates reach
 * draw_glyph, fill_rect and text_xy offset by 0x1f40; what that offset
 * selects in the callees is not established here.
 *
 * huge_move takes an unsigned count here: every call pushes one word for it
 * (add sp,0xa after two far pointers).
 */

#include "dqk.h"

extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

void far screen_draw_begin(char full_redraw);
void far screen_flush(void);

static char open_mode = '1';                                     /* 1cb4 */
static char text_on = 0;                                         /* 1cb5 */
static char flush_mode = 0;                                      /* 1cb6 */
static char pal_dirty = 0;                                       /* 1cb7 */
static long cur_used = -1L;                                      /* 1cb8 */

static char pal_buf[96];                                         /* a2ae */
static struct Res far *cur_res;                                  /* a30e */
static char far *open_name;                                      /* a312 */
static char lib_open;                                            /* a316 */
static int tile_bank;                                            /* a317 */
static char rows_saved[5][144];                                  /* a319 */
static struct Res resources[10];                                 /* a5e9 */
static int tile_slot;                                            /* a625 */
static int tile_set_num;                                         /* a627 */
static char tile_load_mode;                                      /* a629 */

void far res_free(struct Res far * far *resource_ref)                      /* 19a4:0009 */
{
    if (*resource_ref != 0) {
        if ((*resource_ref)->slot >= 0)
            slot_close((*resource_ref)->slot);
        (*resource_ref)->slot = -1;
        *resource_ref = 0;
    }
}

char far tile_load(int file_handle)                                       /* 19a4:004b */
{
    int item_index;
    long pos;

    if ((item_index = lib_find(file_handle, tile_set_num)) <= 0 &&
        (item_index = lib_find(file_handle, tile_bank * 100 + tile_set_num)) <= 0)
        return 0;
    if ((pos = lib_find_item(file_handle, item_index)) < 0)
        return 0;
    if (tile_load_mode) {
        grp_pop();
        lib_create(open_name, tile_slot, 0x454c4954L);
        lib_insert(tile_slot, 0, 0L);
        if (lib_read_convert(file_handle, pos, tile_slot, 0, 0x454c4954L)) {
            if (tile_load_mode != 2)
                return 1;
            lib_insert(tile_slot, 1, 0L);
            if (file_seek(file_handle, 0L, 0) >= 0 &&
                (item_index = lib_find(file_handle, tile_set_num + 0x80)) > 0 &&
                (pos = lib_find_item(file_handle, item_index)) >= 0 &&
                lib_read_convert(file_handle, pos, tile_slot, 1, 0x454c4954L))
                return 1;
        }
        slot_close(tile_slot);
        slot_release(open_name);
        return 0;
    }
    if (!lib_load(file_handle, pos, tile_slot))
        return 0;
    return 1;
}

void far set_open_mode(char mode)                                /* 19a4:01d1 */
{
    open_mode = mode;
}

void far tile_open_file(char far *name, unsigned char set, char kind, char depth,
                        struct Res far * far *resource_ref)                /* 19a4:01dc */
{
    short bank;
    char buf[80];
    char tmp[10];
    int index;
    int len;

    if (*resource_ref != 0)
        res_free(resource_ref);
    for (index = 0; index < 10 && resources[index].slot >= 0; index++)
        ;
    *resource_ref = &resources[index];
    (*resource_ref)->slot = index + 2;
    (*resource_ref)->index = set;
    len = str_len(name) - 1;
    if (name[len] >= '0' && name[len] <= '9') {
        (*resource_ref)->index = 0;
        bank = name[len] - '0';
        str_copy(tmp, name);
        tmp[len] = 0;
        str_format(buf, "%s.tlb", tmp);
        str_copy(tmp, name);
        tmp[4] = 0;
        str_format(tmp, "%s%d%03d", tmp, bank, set);
        if (!slot_open(tmp, index + 2)) {
            lib_open = 1;
            open_name = tmp;
            tile_set_num = set;
            tile_slot = index + 2;
            tile_bank = bank;
            tile_load_mode = kind == 3;
            if (tile_load_mode && depth == 15)
                tile_load_mode = 2;
            file_open(open_mode, buf, 0, tile_load);
        } else
            lib_open = 0;
    } else
        glyph_library_open(open_mode, name, index + 2);
}

void far tile_open_set(char bank, char far *name, int set, int slot) /* 19a4:038a */
{
    char tmp[80];
    char buf[80];

    str_format(buf, "%s.tlb", name);
    str_format(tmp, "%s%d", name, set);
    if (!slot_open(tmp, slot)) {
        lib_open = 1;
        open_name = tmp;
        tile_set_num = set;
        tile_slot = slot;
        tile_bank = bank - '0';
        tile_load_mode = 0;
        file_open(bank, buf, 0, tile_load);
    } else
        lib_open = 0;
}

char far tile_opened(void)                                       /* 19a4:042d */
{
    return lib_open;
}

void far res_alloc(struct Res far * far *resource_ref, unsigned remaining_items)         /* 19a4:0431 */
{
    int index;

    if (*resource_ref != 0)
        res_free(resource_ref);
    for (index = 0; index < 10 && resources[index].slot >= 0; index++)
        ;
    *resource_ref = &resources[index];
    index += 2;
    (*resource_ref)->slot = index;
    (*resource_ref)->index = -1;
    lib_create("activ", index, 0x454c4954L);
    while (remaining_items-- > 0)
        lib_insert(index, 0, 0L);
}

int far res_item_count(struct Res far *resource)                              /* 19a4:04c7 */
{
    return lib_items(slot_base(resource->slot));
}

void far glyph_cell(int col, int row, int glyph, char unused,
                    struct Res far *resource)                           /* 19a4:04e1 */
{
    screen_draw_begin(1);
    draw_glyph(row * 4 + 0x1f44, col * 4 + 0x1f44, resource->slot, glyph);
}

void far glyph_at(int col, int row, unsigned char glyph, char unused,
                  struct Res far *resource)                             /* 19a4:0515 */
{
    screen_draw_begin(1);
    draw_glyph(row, col, resource->slot, glyph);
}

void far glyph_put(int col, int row, unsigned char glyph, char unused,
                   struct Res far *resource)                            /* 19a4:053c */
{
    draw_glyph(row, col, resource->slot, glyph);
}

void far res_glyph_mirror(struct Res far *resource, unsigned char glyph_index)            /* 19a4:055a */
{
    glyph_mirror(slot_base(resource->slot), glyph_index);
}

void far res_draw(int col, int row, struct Res far *resource)           /* 19a4:057a */
{
    draw_glyph(row * 4 + 0x1f40, col * 4 + 0x1f40, resource->slot, resource->index);
}

void far glyph_draw(int col, int row, int glyph, struct Res far *resource) /* 19a4:05a6 */
{
    draw_glyph(row * 4 + 0x1f40, col * 4 + 0x1f40, resource->slot, glyph);
}

int far res_index_find(struct Res far *resource, int item_id)                       /* 19a4:05d1 */
{
    return lib_index_find(slot_base(resource->slot), item_id);
}

void far screen_draw_begin(char full_redraw)                                   /* 19a4:05ee */
{
    if (!text_on && flush_mode != 2) {
        if (full_redraw)
            screen_page_save();
        set_text_mode(0);
        text_on = 1;
    }
}

void far screen_flush_mode_set(char mode)                                 /* 19a4:061a */
{
    if (mode == 2)
        screen_flush();
    flush_mode = mode;
}

void far pal_select(struct Res far *resource)                           /* 19a4:0630 */
{
    long used;

    if (resource != 0)
        used = slot_used(resource->slot);
    else
        used = -1L;
    if (cur_res != resource || used != cur_used) {
        pal_dirty = 1;
        cur_res = resource;
        cur_used = used;
        if (resource != 0)
            huge_move(lib_item(slot_base(resource->slot), 0) + 8, pal_buf, 0x60);
    }
}

void far screen_flush(void)                                      /* 19a4:06d3 */
{
    char pal[96];

    if (flush_mode == 0) {
        if (pal_dirty && cur_res != 0)          /* slot 0, not cur_res's; */
            huge_move(lib_item(slot_base(0), 0) + 8, pal, 0x60); /* never read */
        if (text_on) {
            screen_page_show();
            set_text_mode(1);
            text_on = 0;
        }
        if (is_xvga() || !pixel_shift())
            pal_commit();
        pal_dirty = 0;
    }
}

void far pal_load(struct Res far *resource)                             /* 19a4:0757 */
{
    char pal[96];

    if (resource != 0)
        huge_move(lib_item(slot_base(resource->slot), resource->index) + 8, pal, 0x60);
    else
        mem_fill(pal, 0x60, 0);
    palette_load(pal);
    mem_move(pal, pal_buf, 0x60);
    pal_dirty = 1;
}

void far pal_fade(char fade, struct Res far *resource)                  /* 19a4:07d6 */
{
    char pal[96];

    if (fade) {
        pal_load(0);
        screen_page_show();
        pal_load(resource);
    } else {
        huge_move(lib_item(slot_base(resource->slot), resource->index) + 8, pal, 0x60);
        palette_page_load(pal, 1);
        mem_move(pal, pal_buf, 0x60);
        pal_dirty = 1;
    }
}

void far res_glyph_recolor(unsigned char far *color_map, int glyph_index, struct Res far *resource) /* 19a4:085e */
{
    glyph_recolor(slot_base(resource->slot), glyph_index, color_map);
}

void far res_copy(struct Res far *src, unsigned char source_item, char map,
                  struct Res far *dst, unsigned char destination_item)          /* 19a4:0881 */
{
    int source_index;
    long size;

    if (src == 0) {
        lib_resize(dst->slot, destination_item, 0L);
        return;
    }
    if (map)
        source_index = lib_index_find(slot_base(src->slot), source_item);
    else
        source_index = source_item;
    size = lib_item_size(slot_base(src->slot), source_index);
    if (destination_item < 0x98 || lib_item_size(slot_base(dst->slot), destination_item) < size)
        lib_resize(dst->slot, destination_item, size);
    huge_move(lib_item(slot_base(src->slot), source_index),
              lib_item(slot_base(dst->slot), destination_item), size);
}

void far res_reset(void)                                         /* 19a4:0984 */
{
    int index;

    for (index = 0; index < 10; index++)
        resources[index].slot = -1;
}

void far cell_fill(int col, int row, int width, unsigned height,
                   unsigned char attr)                           /* 19a4:09a5 */
{
    fill_rect(((row + 8) >> 1) + 0x1f40, col * 4 + 0x1f44,
              ((row + height + 8) >> 1) + 0x1f40, (col + width) * 4 + 0x1f44, attr);
}

void far rows_save(int row, int col, int height, int width)      /* 19a4:09eb */
{
    int index;
    short pixel_height;

    text_xy(row, col, &row, &col);
    if (pixel_shift() == 3)
        col &= 0xfff0;
    text_xy(height + 0x1f40, width + 0x1f40, &pixel_height, &width);
    pointer_hide_rect(row, col, row + pixel_height, col + width);
    width = (width >> 3) << (3 - pixel_shift());
    if (width & 1)
        width++;
    video_address_set(row, col);
    for (index = 0; index < video_planes(); index++) {
        video_plane_set(index);
        grab_words(rows_saved[index], pixel_height, width);
    }
    pointer_show();
}

void far rows_restore(int row, int col, int height, int width)   /* 19a4:0aba */
{
    int index;
    short pixel_height;

    text_xy(row, col, &row, &col);
    if (pixel_shift() == 3)
        col &= 0xfff0;
    text_xy(height + 0x1f40, width + 0x1f40, &pixel_height, &width);
    pointer_hide_rect(row, col, row + pixel_height, col + width);
    width = (width >> 3) << (3 - pixel_shift());
    if (width & 1)
        width++;
    video_address_set(row, col);
    for (index = 0; index < video_planes(); index++) {
        video_plane_set(index);
        blit_bytes(rows_saved[index], pixel_height, width, 0);
    }
    pointer_show();
}

void far res_palette_load(struct Res far *resource)                             /* 19a4:0b8c */
{
    if (is_xvga() || !pixel_shift())
        glyph_palette_load(slot_base(resource->slot), lib_itype(slot_base(resource->slot)) != 0);
}

void far *far tile_loader_identity(int unused_arg_a, int unused_arg_b, int unused_arg_c, void far *data)         /* 19a4:0bd9 */
{
    return data;
}
