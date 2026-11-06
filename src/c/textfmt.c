/* DQK.EXE resident module 2d22: text_format, MicroMagic's formatter, and the
 * three helpers it calls near.  text_format hands each character of fmt to
 * put; a '%' escape reads an optional decimal count (a leading 0 pads numbers
 * with '0') and a letter, with v, the int at the argument pointer, as its
 * operand.  d u x X o print v, ld lu lx lo a long, s a string, w the two
 * bytes of v; c puts v's low byte, or with a count above 1 count - 2 NULs
 * and then v's high and low bytes; r formats another string with its own
 * arguments; z puts count NULs, and any other letter is put count times.
 * g moves the argument pointer to the argument its number names; ( ) repeat
 * the text between them v times, and [ ; : ] keep alternative v (from 0) of
 * those between them, a %: starting the fallback.  + - * / # \ & | ^ < >
 * combine v with the count, ~ ignores it, and = sets v to the number given
 * (0 when there is none); each reads the next escape at once.  A * with no
 * number is printf's width: v becomes the count, the next int becomes v,
 * and the next letter is dispatched at once.  The k-th letter of codes calls
 * instead the k-th far handler passed after codes.
 *
 * EXE 0x01f627..0x01fc4b, one Turbo C large-model _TEXT segment starting at
 * byte 7 of paragraph 2d22.  Built with Turbo C 2.01, tcc -ml -O -Z -d.
 * Functions are defined in shipped order; bake consumes them in that order.
 * fmtout.c's last routine ends at 0x01f627.  The routine at 0x01fc4b is
 * another module, 2d84: its switch table offset is from paragraph 2d84, and
 * it is far-called as 1d84:000b.  The module names no DGROUP address, so it
 * has no _DATA and no _BSS.
 *
 * The switch on the escape letter and the one on the letter after %l are
 * Turbo C's sparse-switch value search through the code segment.  The long
 * division and remainder in put_ulong call the runtime's LUDIV@ (00d7:2a76)
 * and LUMOD@ (00d7:2a80).  put_uint and put_ulong are defined old-style, so
 * their callers promote the char zero with cbw; code_index's prototyped char
 * c is pushed as a word.
 */

union argp {
    int       far *i;
    long      far *l;
    char far *far *s;
    void far *far *p;
};

typedef void (far *CodeFn)(int v, int n, void (far *put)(int),
                           union argp far *ap, char far *far *fmt);

static int near pascal code_index(char far *codes, char format_code)       /* 2d22:0007 */
{
    register int index;

    for (index = 1; *codes; index++)
        if (*codes++ == format_code)
            return index;
    return 0;
}

static void near pascal put_uint(value, width, zero, radix, put) /* 2d22:0033 */
register unsigned value;
short width;
char zero;
register unsigned radix;
void (far *put)(int);
{
    if (value >= radix)
        put_uint(value / radix, width - 1, zero, radix, put);
    else
        while (--width > 0)
            put(zero ? '0' : ' ');
    value = value % radix;
    put((value > 9 ? '7' : '0') + value);
}

static void near pascal put_ulong(value, width, zero, radix, put) /* 2d22:0099 */
unsigned long value;
short width;
char zero;
register unsigned radix;
void (far *put)(int);
{
    if (radix <= value)
        put_ulong(value / radix, width - 1, zero, radix, put);
    else
        while (--width > 0)
            put(zero ? '0' : ' ');
    radix = value % radix;
    put((radix > 9 ? '7' : '0') + radix);
}

void far text_format(char far *fmt, void far *args,
                     void (far *put)(), char far *codes, ...)    /* 2d22:011d */
{
    char format_char;
    char zero;
    char far *string_cursor;
    char far *loop_start;
    short format_kind;                            /* handler index, then radix */
    union argp ap;
    short count;
    short loops;
    int far *mark;
    long far *long_argument;
    register int argument_value;
    register int field_width;

    ap.i = args;
    loops = 0;
    while ((format_char = *fmt++) != 0) {
        if (format_char == '%') {
            argument_value = *ap.i;
again:
            format_char = *fmt++;
            if (format_char >= '0' && format_char <= '9') {
                zero = format_char == '0';
                field_width = format_char - '0';
                while ((format_char = *fmt++) >= '0' && format_char <= '9')
                    field_width = field_width * 10 + format_char - '0';
                count = field_width;
            } else {
                zero = field_width = 0;
                count = 1;
            }
            if (codes && (format_kind = code_index(codes, format_char)) != 0) {
                ((CodeFn far *)&codes)[format_kind](argument_value, field_width, put, &ap, &fmt);
                continue;
            }
dispatch:
            switch (format_char) {
            case 'd':
                if (argument_value < 0) {
                    put('-');
                    field_width--;
                    argument_value = -argument_value;
                }
            case 'u':
                put_uint(argument_value, field_width, zero, 10, put);
                ap.i++;
                break;
            case 's':
                string_cursor = *ap.s++;
                if (string_cursor)
                    while (*string_cursor) {
                        put(*string_cursor++);
                        field_width--;
                    }
                while (field_width-- > 0)
                    put(' ');
                break;
            case 'c':
                if (field_width > 1) {
                    while (field_width > 2) {
                        put(0);
                        field_width--;
                    }
                    put(argument_value >> 8);
                }
                put(argument_value);
                ap.i++;
                break;
            case 'w':
                while (field_width > 2) {
                    put(0);
                    field_width--;
                }
                put(argument_value);
                put(argument_value >> 8);
                ap.i++;
                break;
            case 'X':
            case 'x':
                put_uint(argument_value, field_width, zero, 16, put);
                ap.i++;
                break;
            case 'o':
                put_uint(argument_value, field_width, zero, 8, put);
                ap.i++;
                break;
            case 'l':
                long_argument = ap.l;
                format_kind = 11;
                switch (*fmt++) {
                case 'd':
                    break;
                case 'u':
                    format_kind = 10;
                    break;
                case 'x':
                    format_kind = 16;
                    break;
                case 'o':
                    format_kind = 8;
                    break;
                default:
                    fmt--;
                }
                if ((format_kind & 1) && *long_argument < 0) {
                    put('-');
                    field_width--;
                    *long_argument = -*long_argument;
                }
                put_ulong(*long_argument++, field_width, zero, format_kind & ~1, put);
                ap.l = long_argument;
                break;
            case 'r':
                string_cursor = *ap.s++;
                text_format(string_cursor, *ap.p++, put, codes, (&codes)[1]); /* first handler only */
                break;
            case '+':
                argument_value += count;
                goto again;
            case '>':
                argument_value >>= count;
                goto again;
            case '<':
                argument_value <<= count;
                goto again;
            case '(':
                loops = argument_value;
                loop_start = fmt;
                ap.i++;
                if (argument_value)
                    break;
                while (*fmt && (*fmt++ != '%' || *fmt != ')'))
                    ;
                if (*fmt)
                    fmt++;
                break;
            case ')':
                if (--loops > 0)
                    fmt = loop_start;
                break;
            case '[':
                mark = ap.i + 1;
                while (*fmt && argument_value > 0)
                    if (*fmt++ == '%')
                        switch (*fmt++) {
                        case ':':
                            goto next;
                        case ';':
                            argument_value--;
                            break;
                        case ']':
close:
                            ap.i = mark;
                            goto next;
                        }
                break;
            case ']':
                goto close;
            case ':':
            case ';':
                while (*fmt && (*fmt++ != '%' || *fmt != ']'))
                    ;
                if (*fmt)
                    fmt++;
                goto close;
            case 'g':
                ap.i = (int far *)args + field_width;
                break;
            case '&':
                argument_value &= count;
                goto again;
            case '*':
                if (field_width == 0) {
                    field_width = argument_value;
                    count = field_width;
                    argument_value = *++ap.i;
                    format_char = *fmt++;
                    goto dispatch;
                }
                argument_value *= count;
                goto again;
            case '-':
                argument_value -= count;
                goto again;
            case '^':
                argument_value ^= count;
                goto again;
            case '/':
                argument_value /= count;
                goto again;
            case '#':
                argument_value %= count;
                goto again;
            case '|':
                argument_value |= count;
                goto again;
            case '~':
                argument_value = ~argument_value;
                goto again;
            case '\\':
                argument_value = (unsigned)argument_value / count;
                goto again;
            case '=':
                argument_value = field_width;
                goto again;
            case 'z':
                format_char = 0;
                goto repeat;
            case 0:
                fmt--;
                format_char = '%';
            default:
repeat:
                while (--count >= 0)
                    put(format_char);
            }
        } else
            put(format_char);
next:   ;
    }
}
