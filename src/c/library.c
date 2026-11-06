/* DQK.EXE resident module 293f: resource libraries.  Opening a .GLB or
 * .BTX library into a slot, the library header and its item index, finding
 * and reading one item straight from the file, converting a loaded library
 * through the converter registered for its signature, and expanding a packed
 * one: five packing methods.  Their decoders live in segment 10d7, reached
 * directly or through lz_expand and lzw_expand here; the text code tables
 * and the LZW dictionary they use are set up here, and 10d7 calls back into
 * lzw_prune.
 *
 * EXE 0x01b7f7..0x01c7de, one Turbo C large-model _TEXT segment starting at
 * byte 7 of paragraph 293f.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  Callee names are placeholders chosen from use; a modNNNN_ name says
 * only which segment the callee lives in.
 *
 * Turbo C far-calls a public function of this module that is not yet
 * defined and reaches it with `push cs; call near` once it is.  lib_init
 * calls lib_register and takes lib_convert's address through a segment
 * fixup, lib_load calls lib_convert, lib_find calls lib_find_item and
 * lib_expand calls lzw_reset and lzw_expand, all ahead of their definitions,
 * so those carry prototypes.  btx_text calls text_tables near ahead of its
 * definition, which only a static function allows.
 *
 * sig_tags, sig_count and sig_convert are public: module 2a3d reads them.
 * _BSS follows the names (Turbo C's symbol-table order), not the
 * declaration order; these names reproduce the shipped da90..dabb layout,
 * chosen with goldbox.bss_order.  sig_tags' [4] is what fits between da9a
 * and sig_count at daaa; lib_register stops at three.
 *
 * 0x42494c48L is the library tag "HLIB" as four bytes.
 */

#define DQK_DEFINES_LIBRARY
#include "dqk.h"

struct Lzw {                    /* the LZW dictionary, codes 0..0x3ff     */
    char used[0x400];           /* +0     per code; 0 is free             */
    int used_sentinel;          /* +400   used[0x400]: lzcodec.asm's free-code scan stops here */
    char suffix[0x400];         /* +402                                   */
    int prefix[0x400];          /* +802                                   */
    int next;                   /* +1002  0x100 after lzw_reset           */
};

extern void far path_default_ext(char far *path, char far *ext, int force); /* 1c60:025c */
extern void far huge_move(void far *src, void far *dst, unsigned n); /* 00d7:08c2 */

void far lib_register(long tag, long (far *converter)());               /* fn(slot, off, len) */
long far lib_convert(int slot, long off, long size);
long far lib_find_item(int file_handle, int item_index);
void far lzw_reset(struct Lzw far *dictionary);
char far *far lzw_expand(unsigned char far *compressed_data, char far *out, int output_length,
                         struct Lzw far *dictionary);
static void text_tables(void);

static char far *dec_table;                                      /* da90 */
static int item_len;                                             /* da94 */
static void far *item_ptr;                                       /* da96 */
long sig_tags[4];                                                /* da9a */
int sig_count;                                                   /* daaa */
static int lib_target;                                           /* daac */
static int item_index;                                           /* daae */
long (far *sig_convert[3])(int slot, long off, long len);        /* dab0 */

void far lib_init(void)                                          /* 293f:0007 */
{
    sig_count = 0;
    lib_register(0x42494c48L, lib_convert);
}

void far lib_done(void)                                          /* 293f:0026 */
{
}

void far lib_register(long tag, long (far *converter)())                /* fn(slot, off, len) */
{                                                                /* 293f:0027 */
    if (sig_count < 3) {
        sig_tags[sig_count] = tag;
        sig_convert[sig_count] = converter;
        sig_count++;
    } else
        error_box("Too many library signatures");
}

char far lib_load(int file_handle, long pos, int slot)                    /* 293f:0071 */
{
    if (grp_read(file_handle, pos) && lib_convert(slot, 0L, slot_used(slot)) >= 0)
        return 1;
    grp_pop();
    return 0;
}

char far lib_load_cb(int file_handle)                                     /* 293f:00b8 */
{
    return lib_load(file_handle, file_size(file_handle), lib_target);
}

void far lib_open(char mode, char far *name, register int slot)  /* 293f:00d5 */
{
    struct LibHdr hdr;
    char path[80];

    str_copy(path, name);
    path_default_ext(path, "GLB", 0);
    if (!slot_open(path, slot)) {
        lib_target = slot;
        file_open(mode, path, 0, lib_load_cb);
    }
    huge_move(slot_base(slot), &hdr, 16);
    if (hdr.tag != 0x42494c48L)
        error_box("LBLoad: Bad Lib: '%s'", path);
}

int far lib_items(void far *lib)                                 /* 293f:0174 */
{
    struct LibHdr hdr;

    huge_move(lib, &hdr, 16);
    if (hdr.tag != 0x42494c48L) {
        error_box("LBCount: Invalid Library File");
        return 0;
    }
    return hdr.count;
}

long far lib_sig(void far *lib)                                  /* 293f:01b6 */
{
    struct LibHdr hdr;

    huge_move(lib, &hdr, 16);
    if (hdr.tag != 0x42494c48L) {
        error_box("LBType: Invalid Library File");
        return 0L;
    }
    return hdr.sig;
}

char far *far lib_item(void far *lib, register int item_index)            /* 293f:01fd */
{
    long off;
    struct LibHdr hdr;

    huge_move(lib, &hdr, 16);
    if (hdr.tag != 0x42494c48L) {
        error_box("LBIBase: Invalid Library File");
        return 0;
    }
    if (item_index < 0 || item_index >= hdr.count) {
        error_box("Invalid item (%d/%d)", item_index, hdr.count);
        return 0;
    }
    huge_move(huge_add(lib, item_index * sizeof(long) + sizeof hdr), &off, 4);
    return huge_add(lib, off);
}

long far lib_item_size(void far *lib, register int item_index)            /* 293f:02a1 */
{
    long off[2];
    struct LibHdr hdr;

    huge_move(lib, &hdr, 16);
    if (hdr.tag != 0x42494c48L) {
        error_box("LBISize: Invalid Library File");
        return 0L;
    }
    if (item_index < 0 || item_index >= hdr.count) {
        error_box("Invalid item (%d/%d)", item_index, hdr.count);
        return 0L;
    }
    huge_move(huge_add(lib, item_index * sizeof(long) + sizeof hdr), off, 8);
    return off[1] - off[0];
}

int far lib_itype(void far *lib)                                 /* 293f:033d */
{
    struct LibHdr hdr;

    huge_move(lib, &hdr, 16);
    if (hdr.tag != 0x42494c48L) {
        error_box("LBIndxType: Invalid Library File");
        return 0;
    }
    return hdr.itype;
}

int far lib_index_find(void far *lib, int id)                    /* 293f:0380 */
{
    int ent[2];
    char far *index_cursor;
    short remaining_entries;

    index_cursor = lib_item(lib, 0);
    huge_move(index_cursor, &remaining_entries, 2);
    index_cursor = huge_add(index_cursor, 2L);
    while (--remaining_entries >= 0) {
        huge_move(index_cursor, ent, 4);
        index_cursor = huge_add(index_cursor, 4L);
        if (ent[0] == id)
            return ent[1];
    }
    return -1;
}

int far lib_find(register int file_handle, int id)                        /* 293f:0419 */
{
    int ent[2];
    long pos;
    short remaining_entries;

    pos = file_seek(file_handle, 0L, 1);
    if (lib_find_item(file_handle, 0) <= 0 || dos_io(0x3f00, &remaining_entries, file_handle, 2) != 2)
        return -1;
    while (--remaining_entries >= 0) {
        if (dos_io(0x3f00, ent, file_handle, 4) != 4)
            return -1;
        if (ent[0] == id) {
            file_seek(file_handle, pos, 0);
            return ent[1];
        }
    }
    file_seek(file_handle, pos, 0);
    return 0;
}

long far lib_find_item(register int file_handle, register int item_index)          /* 293f:04c9 */
{
    struct LibHdr hdr;
    long start;
    long off[2];
    long pos;

    start = file_seek(file_handle, 0L, 1);
    if (dos_io(0x3f00, &hdr, file_handle, 16) != 16)
        return -1L;
    if (hdr.tag != 0x42494c48L || hdr.method != 0) {
        error_box("Invalid Library File (%d)", hdr.method);
        return -1L;
    }
    if (item_index < 0 || item_index >= hdr.count) {
        error_box("Invalid item (%d/%d)", item_index, hdr.count);
        return 0L;
    }
    pos = item_index * sizeof(long) + start + sizeof hdr;
    if (file_seek(file_handle, pos, 0) != pos || dos_io(0x3f00, off, file_handle, 8) != 8 ||
        file_seek(file_handle, start + off[0], 0) < 0)
        return -1L;
    return off[1] - off[0];
}

void far btx_open(char mode, char far *name, int slot)           /* 293f:05e2 */
{
    char path[80];

    str_copy(path, name);
    path_default_ext(path, "BTX", 0);
    lib_open(mode, path, slot);
}

void far btx_text(int slot, int item_index, char far *dst)                /* 293f:0626 */
{
    char far *lib;
    int len;

    lib = slot_base(slot);
    len = lib_item_size(lib, item_index);
    lib = lib_item(lib, item_index);
    huge_move(lib, dst, len);
    if (*dst & 0x80) {
        len = (dst[0] & 0x7f) << 8 | dst[1] & 0xff;
        text_tables();
        huff_expand(lib + 2, dst, len, db_buffer());
    }
    dst[len] = 0;
}

char far item_read_cb(int file_handle)                                    /* 293f:06c2 */
{
    return (item_len = lib_find_item(file_handle, item_index)) > 0 &&
           dos_io(0x3f00, item_ptr, file_handle, item_len) == item_len;
}

int far lib_read_item(char mode, char far *name, int requested_item, void far *buf) /* 293f:06ff */
{
    char path[80];

    item_index = requested_item;
    item_ptr = buf;
    str_copy(path, name);
    path_default_ext(path, "GLB", 0);
    file_open(mode, path, 0, item_read_cb);
    return item_len;
}

char far table_read_cb(int file_handle)                                   /* 293f:075d */
{
    int len;

    if (dec_table == 0) {               /* shipped: a second call   */
        len = file_size(file_handle);         /* reads with len unset     */
        if ((dec_table = mem_alloc(len)) == 0) {
            error_box("Out of memory on LBDecode");
            return 0;
        }
    }
    return dos_io(0x3f00, dec_table, file_handle, len) == len;
}

void far table_load(void)                                        /* 293f:07bd */
{
    file_open('A', "table.dec", 0, table_read_cb);
}

void far make_table(char far *out, ...)                          /* 293f:07d6 */
{
    void *argument_cursor;
    int bits;
    int count;
    char far *text_cursor;
    char **string_argument;
    int output_length;

    output_length = 0;
    argument_cursor = ...;                           /* stdarg.h's va_start and  */
    for (;;) {
        bits = *((int *)argument_cursor)++;          /* va_arg, written out      */
        if (bits == 0)
            break;
        string_argument = argument_cursor;
        text_cursor = *string_argument++;
        argument_cursor = string_argument;
        for (; *text_cursor != 0; text_cursor++) {
            count = 1 << (8 - bits);
            while (--count >= 0) {
                *out = *text_cursor;
                out[0x100] = bits;
                output_length++;
                out++;
            }
        }
    }
    out[0x100] = 0x87;
    out++;
    output_length++;
    out[0x100] = 0x87;
    out++;
    output_length++;
    out += 0x100;
    for (count = 0; count < 0x100; count++, out++) {
        *out = count;
        out[0x100] = 8;
    }
    if (output_length != 0x100)
        error_box("MakeTable count %d/256", output_length);
}

static void text_tables(void)                                    /* 293f:08b3 */
{
    make_table(db_buffer(), 3, " e", 4, "t", 5, "oansirhlu", 6, "dmcywf.pg,",
               8, "\n\rbIvETANkSOR'LCD-GHWMPUBFYx?qKVj01)(z43!%X5287Z*9/&6J#^+;Q:\"\t", 0);
}

char far *far lz_expand(char far *compressed_data, char far *out, int output_length)      /* 293f:08f2 */
{
    out = (char far *)((long)out + 0xf8008000L); /* seg-800, off+8000 for off < 8000 */
    return lz_decode(compressed_data, out, output_length);
}

void far lib_expand(register int slot, long off, long size, long full, int method)
{                                                                /* 293f:091c */
    char far *compressed_data;
    char far *out;
    long delta;
    long left;
    short packed_length;
    register int chunk;

    left = full - 16;
    delta = full - size;
    out = huge_add(slot_base(slot), off + 16);
    huge_move(out, &packed_length, 2);
    delta += packed_length;
    slot_insert_delete(slot, off + 16, delta);
    out = huge_add(slot_base(slot), off + 16);
    compressed_data = huge_add(out, delta + 2);
    if (method == 3)
        lzw_reset(db_buffer());
    else if (method == 4)
        text_tables();
    while (left > 0) {
        chunk = left > 0x2000 ? 0x2000 : left;
        switch (method) {
        case 1:
            compressed_data = huge_add(packbits_expand(compressed_data, out, chunk), 0L);
            break;
        case 2:
            compressed_data = huge_add(huff_expand(compressed_data, out, chunk, dec_table), 0L);
            break;
        case 3:
            compressed_data = huge_add(lzw_expand(compressed_data, out, chunk, db_buffer()), 0L);
            break;
        case 4:
            compressed_data = huge_add(huff_expand(compressed_data, out, chunk, db_buffer()), 0L);
            break;
        case 5:
            compressed_data = huge_add(lz_expand(compressed_data, out, chunk), 0L);
            break;
        }
        out = huge_add(out, chunk);
        left -= chunk;
    }
    slot_insert_delete(slot, off + full, -(long)packed_length);
}

long far lib_convert(register int slot, long off, long size)     /* 293f:0b9c */
{
    struct LibHdr hdr;
    char far *library_data;
    long prev;
    long cur;
    long got;
    long adj;
    long (far *converter)(int slot, long off, long len);
    register int index;

    library_data = huge_add(slot_base(slot), off);
    huge_move(library_data, &hdr, 16);
    if (hdr.tag != 0x42494c48L)
        error_box("_LBConvert: Bad Lib Header");
    if (hdr.method != 0) {
        lib_expand(slot, off, size, hdr.size, hdr.method);
        size = hdr.size;
        hdr.method = 0;
        library_data = huge_add(slot_base(slot), off);
    }
    if (hdr.size & 1)
        hdr.size++;
    if (hdr.tag != 0x42494c48L || hdr.size != size) {
        error_box("Invalid library header");
        return -1L;
    }
    for (index = 0; sig_tags[index] != hdr.sig && index < sig_count; index++)
        ;
    if (index < sig_count) {
        converter = sig_convert[index];
        library_data = huge_add(library_data, 16L);
        huge_move(library_data, &cur, 4);
        adj = 0;
        index = 0;
        while (index < hdr.count) {
            prev = cur;
            library_data += 4;
            huge_move(library_data, &cur, 4);
            cur += adj;
            if (index != 0 || hdr.itype == 0) {
                got = (*converter)(slot, off + prev, cur - prev);
                if (got < 0)
                    return 0L;
                adj += got - cur + prev;
                cur = prev + got;
            }
            library_data = huge_add(slot_base(slot), ++index * sizeof(long) + off + sizeof hdr);
            huge_move(&cur, library_data, 4);
        }
        hdr.size += adj;
    }
    huge_move(&hdr, huge_add(slot_base(slot), off), 16);
    return hdr.size;
}

int far lzw_prune(int keep, struct Lzw far *dictionary, int low)          /* 293f:0e39 */
{
    int index;
    int prune_iteration;
    char far *usage_cursor;

    prune_iteration = 0;
    do {
        for (index = 0x100, usage_cursor = dictionary->used + index; index < 0x400; index++, usage_cursor++) {
            if (*usage_cursor < 0 && index != keep) {
                *usage_cursor = 0;
                prune_iteration++;
                if (index < low)
                    low = index;
            } else if (*usage_cursor > 0)
                *usage_cursor = -1;
        }
        for (index = 0x100, usage_cursor = dictionary->used + index; index < 0x400; index++, usage_cursor++)
            if (*usage_cursor != 0)
                dictionary->used[dictionary->prefix[index]] = 1;
    } while (prune_iteration < 0x55);
    return low;
}

void far lzw_reset(struct Lzw far *dictionary)                            /* 293f:0ed2 */
{
    short index;

    mem_fill(dictionary->used, 0x100, -1);
    mem_fill(dictionary->used + 0x100, 0x300, 0);
    mem_fill(&dictionary->used_sentinel, 2, 0);
    for (index = 0; index < 0x100; index++)
        dictionary->suffix[index] = index;
    mem_fill(dictionary->prefix, 0x200, -1);
    dictionary->next = 0x100;
}

char far *far lzw_expand(unsigned char far *compressed_data, char far *out, int output_length,
                         struct Lzw far *dictionary)                      /* 293f:0f6a */
{
    unsigned short packed_word;
    char far *end;
    int code;

    end = out + output_length;
    packed_word = *compressed_data << 8;
    compressed_data++;
    packed_word |= *compressed_data;
    compressed_data++;
    code = packed_word >> 6;
    lzw_emit(&out, code, dictionary);
    return compressed_data = lzw_decode(packed_word, code, compressed_data, dictionary->next, end, dictionary, &out);
}
