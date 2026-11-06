/* DQK.EXE resident module 2d84: str_to_num, reading a number whose radix is
 * named by a letter after its digits.  A trailing B, O, D or H (either case)
 * selects binary, octal, decimal or hexadecimal and is not read as a digit;
 * with any other last character the whole string is decimal.  Digits valid
 * in the radix accumulate; a '-' anywhere before the end negates the result
 * (a second does not cancel it); other characters up to ' ' are skipped; a
 * digit the radix does not allow, any other character above ' ' and any byte
 * from 0x80 end the number.
 *
 * EXE 0x01fc4b..0x01fd8a, one Turbo C large-model _TEXT segment starting at
 * byte 0x0b of paragraph 2d84: textfmt.c's last retf is the byte before, and
 * six zero bytes after the range pad to the RTLink manager at 0x01fd90.
 * Built with Turbo C 2.01, tcc -ml -O -Z -d.  The switch's value table sits
 * at 2d84:0049 and is addressed from the paragraph, which is why the routine
 * is an object of its own.  lineedit.c's num_edit far-calls it as 1d84:000b.
 * The module names no DGROUP address, so it has no _DATA and no _BSS.
 *
 * The switch on the radix letter is Turbo C's sparse-switch value search
 * through the code segment (2e 3b 07 at 2d84:003a).
 */

int far str_to_num(char far *text)                                  /* 2d84:000b */
{
    char digit_char;
    char far *text_cursor;
    char far *end;
    char neg;
    register unsigned radix;
    register unsigned numeric_value;

    for (end = text; *end; end++)
        ;
    end--;
    switch (*end) {
    case 'H': case 'h':
        radix = 16;
        break;
    case 'O': case 'o':
        radix = 8;
        break;
    case 'D': case 'd':
        radix = 10;
        break;
    case 'B': case 'b':
        radix = 2;
        break;
    default:
        radix = 10;
        end++;
    }
    numeric_value = 0;
    neg = 0;
    for (text_cursor = text; text_cursor < end; ) {
        digit_char = *text_cursor++;
        if (digit_char >= '0' && digit_char <= '9' && digit_char < radix + '0')
            numeric_value = radix * numeric_value + digit_char - '0';
        else if (digit_char >= 'A' && digit_char < radix + 'A' - 10)
            numeric_value = radix * numeric_value + digit_char - ('A' - 10);
        else if (digit_char >= 'a' && digit_char < radix + 'a' - 10)
            numeric_value = radix * numeric_value + digit_char - ('a' - 10);
        else if (digit_char == '-')
            neg = 1;
        else if ((unsigned char)digit_char > ' ')
            break;
    }
    return neg ? -numeric_value : numeric_value;
}
