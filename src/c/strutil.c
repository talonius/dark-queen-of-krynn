/* DQK.EXE resident module 1df9: string, number and message helpers.
 *
 * EXE 0x010391..0x010966, one Turbo C large-model _TEXT segment starting at
 * byte 1 of paragraph 1df9.  Built with Turbo C 2.01, tcc -ml -O -Z -d: -d
 * merges duplicate strings, which is what puts msg_text's "" on the NUL of
 * "%d" (DGROUP 0x1d46 = 0x1d44 + 2).  Functions are defined in shipped order;
 * bake consumes them in that order.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call.  The callees in segment 00d7 belong to MicroMagic's own library rather
 * than Turbo C's: str_find() takes its character argument as a char, which the
 * Turbo C 2.01 strchr() prototype (int c) would not push with `mov al,0`.
 *
 * The statics' names fix their DGROUP order: Turbo C 2.01 emits _BSS in its
 * symbol-table order, which follows the name and not the declaration order.
 * These names reproduce the shipped 0xa7ba num, 0xa7bc ptr, 0xa7c0 table,
 * 0xa7c4 buffer; the originals are not recoverable from it.
 */

#include "dqk.h"

extern int  far str_match(char far *text_cursor, char far *pattern_cursor);           /* 1d10:006d */

int far char_upper(char character_code)                                       /* 1df9:0001 */
{
    return to_upper(character_code);
}

char far char_abs(char value)                                        /* 1df9:0011 */
{
    char result;

    if (value > 0)
        result = value;
    else
        result = -value;
    return result;
}

unsigned far umin(unsigned left_value, unsigned right_value)                        /* 1df9:0034 */
{
    unsigned short result;

    if (left_value > right_value)
        result = right_value;
    else
        result = left_value;
    return result;
}

void far int_to_str(int value, char far *buf)                    /* 1df9:0057 */
{
    str_format(buf, "%d", value);
}

static char string_buf[82];              /* a7c4; sound.c _BSS starts a816 */

char far *far substr(char far *text, int start, int len)            /* 1df9:0071 */
{
    mem_move(text + start - 1, string_buf, len);
    string_buf[len] = 0;
    return string_buf;
}

int far str_length(char far *text)                                  /* 1df9:009e */
{
    return str_len(text);
}

int far str_index(char far *pattern, char far *text_cursor)                      /* 1df9:00b0 */
{
    register int index;

    for (index = 0; *text_cursor; text_cursor++, index++)
        if (str_match(pattern, text_cursor))
            return index;
    return 0;
}

void far str_to_uint(char far *text, long far *out, int far *err)   /* 1df9:00e6 */
{
    unsigned short digit;
    unsigned short place;
    unsigned short scan_index;
    char far *digit_cursor;

    *out = 0;
    digit = place = 0;
    for (digit_cursor = str_find(text, 0) - 1; digit_cursor >= text; place++, digit_cursor--) {
        if (*digit_cursor >= '0' && *digit_cursor <= '9') {
            digit = *digit_cursor - '0';
            for (scan_index = place; scan_index > 0; scan_index--)
                digit *= 10;
            *out += digit;
        } else {
            *err = str_len(text) - place - 1;
            return;
        }
    }
    *err = 0;
}

void far str_to_long(char far *text, long far *out, int far *err)   /* 1df9:0195 */
{
    long digit;
    unsigned short place;
    unsigned short scan_index;
    char far *digit_cursor;

    *out = 0;
    digit = place = 0;
    for (digit_cursor = str_find(text, 0) - 1; digit_cursor >= text; place++, digit_cursor--) {
        if (*digit_cursor >= '0' && *digit_cursor <= '9') {
            digit = *digit_cursor - '0';
            for (scan_index = place; scan_index > 0; scan_index--)
                digit *= 10;
            *out += digit;
        } else {
            *err = str_len(text) - place - 1;
            return;
        }
    }
    *err = 0;
}

void far str_insert(char far *inserted_text, char far *destination, int pos)           /* 1df9:025d */
{
    register int len;
    char far *insertion_at;

    len = str_len(inserted_text);
    insertion_at = destination + pos - 1;
    mem_move(insertion_at, insertion_at + len, str_len(insertion_at) + 1);
    mem_move(inserted_text, insertion_at, len);
}

void far str_delete(char far *text, register int pos, int delete_length)        /* 1df9:02bd */
{
    mem_move(text + pos + delete_length - 1, text + pos - 1, str_len(text + pos + delete_length - 1) + 1);
}

void far pool_alloc(void far * far *out, int unused, struct Pool far *pool) /* 1df9:02fc */
{
    unsigned char bit;
    unsigned char row;
    unsigned char byte;
    register int slot;

    for (row = 0; (byte = pool->bitmap[row]) == 0xff && row < pool->capacity - 1; row++)
        ;
    for (bit = 0; bit < 8 && (byte & (1 << bit)); bit++)
        ;
    slot = row * 8 + bit;
    if (pool->capacity <= slot) {
        *out = 0;
    } else {
        pool->bitmap[row] |= 1 << bit;
        *out = pool->base + pool->stride * slot;
    }
}

void far pool_free(void far *ptr, int unused, struct Pool far *pool) /* 1df9:03c1 */
{
    unsigned char lo;
    unsigned char hi;
    register int slot;

    slot = ((char far *)ptr - pool->base) / pool->stride;
    if (slot >= 0 && pool->capacity > slot) {
        hi = slot >> 3;
        lo = slot - hi * 8;
        pool->bitmap[hi] &= ~(1 << lo);
    }
}

char far pool_lowbit(int value)                                  /* 1df9:0428 */
{
    return value & 1;
}

void far mem_move5(void far *src, void far *dst, unsigned byte_count)     /* 1df9:0432 */
{
    mem_move(src, dst, byte_count);
}

int far random_n(int upper_bound)                                          /* 1df9:044d */
{
    return random_int(upper_bound);
}

char far screen_sync(void)                                       /* 1df9:045b */
{
    screen_flush();
    return key_poll();
}

unsigned char far hi_byte(int value)                                 /* 1df9:0466 */
{
    return (value >> 8) & 0xff;
}

unsigned char far lo_byte(int value)                                 /* 1df9:0474 */
{
    return value & 0xff;
}

static int msg_num;                     /* a7ba */
static char far *msg_ptr;               /* a7bc */
static char far * far *msg_table;       /* a7c0 */

void far msg_set_table(int count, char far * far *table)         /* 1df9:047e */
{
    msg_num = count;
    msg_table = table;
}

char far *far msg_text(register int message_index)                           /* 1df9:0494 */
{
    if (msg_table && message_index >= 0 && message_index < msg_num && msg_table[message_index])
        return msg_table[message_index];
    return "";
}

void far msg_putc(char character_code)                                        /* 1df9:04e0 */
{
    *msg_ptr++ = character_code;
}

char far *far msg_format(char far *fmt, ...)                     /* 1df9:04f3 */
{
    msg_ptr = string_buf;
    text_format(fmt, (void far *)(&fmt + 1), msg_putc, 0L);
    *msg_ptr = 0;
    return string_buf;
}

void far delay(unsigned milliseconds)                                      /* 1df9:052a */
{
    long until;
    register int rem;

    if (milliseconds == 0)
        return;
    screen_flush();
    until = milliseconds;
    until *= 72;
    rem = until % 1000;
    until /= 1000;
    if (random_int(1000) < rem)
        until++;
    until += timer_ticks();
    while (timer_ticks() < until) {
        if (is_xvga() || !pixel_shift())
            input_poll();
    }
}
