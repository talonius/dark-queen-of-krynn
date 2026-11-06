/* DQK.EXE overlay 04 module 26f8: the manual-word check in the play loop.
 * journal_check picks one of twenty headings in the adventurer's journal
 * and one of its three listed words, prints the page and which word after
 * the heading to type, and reads the reply; after three wrong replies it
 * gives up.  It returns 1 when the reply matched, else 0.  The headings
 * and words are stored scrambled -- reversed, each character raised by
 * the word's length -- and unscramble undoes that.  Each stage it reaches
 * sets one of copyprot_markers[1..4]; game_main reads [1..3].
 *
 * EXE 0x038e53..0x039134, one Turbo C large-model _TEXT segment starting at
 * byte 3 of paragraph 26f8 in RTLink section 4 (payload 0x37910, link
 * segment 25a4).  The first 3 bytes of paragraph 26f8 are the tail of
 * module 264f, and the last 4 bytes of the range lie in paragraph 2726,
 * whose module starts at 2726:0004.  Built with Turbo C 2.01,
 * tcc -ml -O -Z -d.  Functions are defined in shipped order; bake consumes
 * them in that order.  Sections 5 to 8 hold other bytes at the 25a4 link
 * segment.
 *
 * The seg:off beside each extern is the far pointer as stored in the shipped
 * call, or the DGROUP offset of an extern variable.  journal_check is
 * reached through the vector stub 1d99:117f, from gamemain.c.
 *
 * _DATA is DGROUP 6dd2..7275: quiz_table, whose 80 far pointers are
 * relocated, closed by a zero entry random_n(20) never picks, then the
 * scrambled words and eight stored literals; "" is stored on the first
 * heading's NUL.
 * _BSS is c6f8..c74d.  Nothing found in the image references unused; it
 * keeps the shipped layout.  Turbo C lays _BSS out by the statics' names,
 * and these names give the shipped order.
 */

#include "dqk.h"

struct Quiz {
    char far *heading;          /* +0  scrambled                          */
    unsigned char page;         /* +4  journal page                       */
    unsigned char word_no[3];   /* +5  word's place after the heading     */
    char far *word[3];          /* +8  scrambled                          */
};

static struct Quiz quiz_table[] = {                              /* 6dd2 */
    /* ABILITY SCORES: EVERY RANDOMLY SCORES */
    { "aS`]Qa.gbWZWPO", 3, { 1, 5, 8 },
      { "^WJ[J", "aTUWLVIZ", "YKXUIY" } },
    /* CHARACTER CLASSES: CLASSES CHARACTER MUST */
    { "dVddR]T1cVeTRcRYT", 4, { 1, 7, 8 },
      { "ZLZZHSJ", "[N]LJ[JQL", "XWYQ" } },
    /* ALIGNMENT: PHILOSOPHY LIVES BY */
    { "]WNVWPRUJ", 6, { 4, 7, 8 },
      { "cRZY]YVSRZ", "XJ[NQ", "[D" } },
    /* OTHER ATTRIBUTES: THREE IMPORTANT VALUES */
    { "cUdeRYbddQ0bUXd_", 7, { 5, 6, 7 },
      { "JJWMY", "]WJ][XYVR", "YK[RG\\" } },
    /* PREPERATION TIPS: BANDED TOGETHER INN */
    { "c`Yd0^_YdQbU`Ub`", 8, { 5, 6, 10 },
      { "JKJTGH", "ZMP\\MOW\\", "QQL" } },
    /* COMBAT: ADVENTURERS BATTLE DANGEROUS */
    { "ZGHSUI", 9, { 1, 3, 8 },
      { "^]P]`_YPaOL", "KRZZGH", "\\^X[NPWJM" } },
    /* INITIATIVE: ROUND DIVIDED SEGMENTS */
    { "O`S^KS^SXS", 9, { 2, 6, 9 },
      { "ISZTW", "KLKP]PK", "[\\VMUOM[" } },
    /* THAC0: ABILITY HIT MELEE */
    { "5HFMY", 9, { 2, 4, 7 },
      { "`[PSPIH", "WLK", "JJQJR" } },
    /* DAMAGE: RANGE POINT LOSS */
    { "KMGSGJ", 10, { 4, 7, 8 },
      { "JLSFW", "YSNTU", "WWSP" } },
    /* RANGED COMBAT: FIRING DISTANT WEAPONS */
    { "aNOZ\\P-QRT[N_", 10, { 4, 6, 9 },
      { "MTOXOL", "[UH[ZPK", "ZUVWHL^" } },
    /* SAVING THROWS: SUCH POISON SPELLS */
    { "`d\\_Ua-T[VcN`", 10, { 2, 4, 6 },
      { "LGYW", "TUYOUV", "YRRKVY" } },
    /* RUNNING AWAY: MAY FLEE FROM */
    { "eMcM,SZUZZa^", 11, { 3, 4, 5 },
      { "\\DP", "IIPJ", "QSVJ" } },
    /* COMBAT STRATEGIES: SUCCEED SKILLED DEPLOYS */
    { "dVZXVeRced1eRS^`T", 11, { 2, 6, 8 },
      { "KLLJJ\\Z", "KLSSPRZ", "Z`VSWLK" } },
    /* WOUNDED CHARACTERS: SERIOUSLY INJURED CURED */
    { "edWfUSdSZU2VWV`gai", 11, { 4, 5, 8 },
      { "bU\\^XR[N\\", "KLY\\QUP", "IJWZH" } },
    /* AFTER COMBAT: IF SURVIVE BATTLEFIELD */
    { "`MNY[O,^Q`RM", 12, { 1, 6, 9 },
      { "HK", "L]P]Y\\Z", "OWPTQPW__LM" } },
    /* MAGIC: INTEGRAL DARK QUEEN */
    { "HNLFR", 12, { 3, 6, 7 },
      { "TIZOM\\VQ", "OVEH", "SJJZV" } },
    /* THE MOONS OF KRYNN: CREATION WORLD GOVERNED */
    { "``kd]2Xa2e`aa_2WZf", 13, { 3, 6, 10 },
      { "VWQ\\IMZK", "IQWT\\", "LMVZM^WO" } },
    /* DIETIES: SINCE EARLIEST WISDOM */
    { "ZLP[LPK", 13, { 1, 3, 8 },
      { "JHSNX", "\\[MQTZIM", "SUJYO]" } },
    /* RANGERS: USE DRUIDIC THEY */
    { "ZYLNUHY", 14, { 2, 5, 7 },
      { "HVX", "JPKP\\YK", "]ILX" } },
    /* MAGICAL TREASURES: TRAVEL ABOUT ENCOUNTER */
    { "dVcfdRVce1]RTZXR^", 14, { 3, 4, 6 },
      { "RK\\GXZ", "YZTGF", "[N]W^XLWN" } },
    { 0 }
};

static char unused;                                              /* c6f8 */
static char answer[41];                                          /* c6f9 */
static unsigned char ntries;                                     /* c722 */
static char response[41];                                        /* c723 */
static unsigned char section;                                    /* c74c */
static unsigned char word_no;                                    /* c74d */

void far unscramble(char far *dst, char far *src)                /* 26f8:0003 */
{
    char buf[42];
    unsigned char index;
    unsigned char len;

    len = str_length(src);
    str_copy(buf, "");
    for (index = len; index >= 1; index--)
        str_copy(buf, msg_format("%s%c", buf, *(src + index - 1) - len));
    str_copy(dst, buf);
}

char far journal_check(void)                                     /* 26f8:008b */
{
    char ok;
    char heading[42];

    mem_zero(copyprot_markers, 5);
    frame_border();
    ntries = 0;
    do {
        ntries++;
        str_copy(answer, "");
        copyprot_markers[1] = 1;
        section = random_n(20);
        do
            word_no = random_n(3);
        while (quiz_table[section].word_no[word_no] == 0xff);
        unscramble(answer, quiz_table[section].word[word_no]);
        status_printf(3, 2, 10, 0, " journal, page ");
        status_printf(0x11, 2, 13, 0, msg_format("%s%s%s", " (",
                      byte_str(quiz_table[section].page), ")\t\t\t\t "));
        status_printf(3, 4, 10, 0, " what is word # ");
        status_printf(0x13, 4, 13, 0, msg_format("%s%s",
                      byte_str(quiz_table[section].word_no[word_no]), "\t\t "));
        status_printf(3, 6, 10, 0, " after the heading ");
        text_clear(4, 8, 0x26, 8);
        unscramble(heading, quiz_table[section].heading);
        status_printf(4, 8, 13, 0, heading);
        str_copy(response, input_text("Enter ", 14, 0, 20));
        copyprot_markers[2] = 1;
    } while (ntries != 3 && !str_same(response, answer));
    copyprot_markers[3] = 1;
    if (str_same(response, answer))
        ok = 1;
    else
        ok = 0;
    copyprot_markers[4] = 1;
    return ok;
}
