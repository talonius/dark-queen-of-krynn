;  dgorder.inc -- every DGROUP segment, in the shipped DGROUP's address order.
;  Included by start.asm (LINKED).  The linker places a class's segments in the
;  order it first sees them, and start.obj is linked first, so declaring them
;  here lays DGROUP out as it shipped; the offsets are the shipped ones.  Each C
;  module is compiled with -zRD_<MODULE> -zDB_<MODULE>, which names its _DATA
;  and _BSS (the code is unchanged); lowlevel.asm and farmem.asm name theirs
;  under LINKED; the B_GLOBALS and B_GAP segments are globals.asm's.  A one-byte
;  hole after an odd-length segment is the next one's word alignment.
;  Generated from src/generated/manifest.json and the modules' objects.

D_STARTDAT      segment word public 'DATA'     ; 0000  44 bytes
D_STARTDAT      ends
D_REST          segment word public 'DATA'     ; 002c  118 bytes
D_REST          ends
D_FXCORE        segment word public 'DATA'     ; 00a2  377 bytes
D_FXCORE        ends
D_HERO          segment word public 'DATA'     ; 021c  111 bytes
D_HERO          ends
D_CHARTAB       segment word public 'DATA'     ; 028c  6695 bytes
D_CHARTAB       ends
D_TILES         segment word public 'DATA'     ; 1cb4  35 bytes
D_TILES         ends
D_TEXTWIN       segment word public 'DATA'     ; 1cd8  47 bytes
D_TEXTWIN       ends
D_LISTBOX       segment word public 'DATA'     ; 1d08  3 bytes
D_LISTBOX       ends
D_MENU          segment word public 'DATA'     ; 1d0c  56 bytes
D_MENU          ends
D_STRUTIL       segment word public 'DATA'     ; 1d44  3 bytes
D_STRUTIL       ends
D_SOUND         segment word public 'DATA'     ; 1d48  275 bytes
D_SOUND         ends
D_SNDEVENT      segment word public 'DATA'     ; 1e5c  1637 bytes
D_SNDEVENT      ends
D_SFXCTL        segment word public 'DATA'     ; 24c2  123 bytes
D_SFXCTL        ends
D_SPELLTAB      segment word public 'DATA'     ; 253e  5493 bytes
D_SPELLTAB      ends
D_CHARPIC       segment word public 'DATA'     ; 3ab4  22 bytes
D_CHARPIC       ends
D_GAMETEXT      segment word public 'DATA'     ; 3aca  7670 bytes
D_GAMETEXT      ends
D_MISC          segment word public 'DATA'     ; 58c0  9 bytes
D_MISC          ends
D_FRAME         segment word public 'DATA'     ; 58ca  13 bytes
D_FRAME         ends
D_COMBAT        segment word public 'DATA'     ; 58d8  158 bytes
D_COMBAT        ends
D_CBTUTIL       segment word public 'DATA'     ; 5976  201 bytes
D_CBTUTIL       ends
D_CBACTION      segment word public 'DATA'     ; 5a40  714 bytes
D_CBACTION      ends
D_CMBSETUP      segment word public 'DATA'     ; 5d0a  170 bytes
D_CMBSETUP      ends
D_AITURN        segment word public 'DATA'     ; 5db4  162 bytes
D_AITURN        ends
D_ROSTER        segment word public 'DATA'     ; 5e56  67 bytes
D_ROSTER        ends
D_GLBDATA       segment word public 'DATA'     ; 5e9a  48 bytes
D_GLBDATA       ends
D_GAMEMAIN      segment word public 'DATA'     ; 5eca  9 bytes
D_GAMEMAIN      ends
D_ECLRUN        segment word public 'DATA'     ; 5ed4  23 bytes
D_ECLRUN        ends
D_SAVELOAD      segment word public 'DATA'     ; 5eec  391 bytes
D_SAVELOAD      ends
D_PICVIEW       segment word public 'DATA'     ; 6074  25 bytes
D_PICVIEW       ends
D_CONFIG        segment word public 'DATA'     ; 608e  201 bytes
D_CONFIG        ends
D_DIRNAMES      segment word public 'DATA'     ; 6158  24 bytes
D_DIRNAMES      ends
D_APPROACH      segment word public 'DATA'     ; 6170  9 bytes
D_APPROACH      ends
D_COMBTAB       segment word public 'DATA'     ; 617a  3070 bytes
D_COMBTAB       ends
D_NEWGAME       segment word public 'DATA'     ; 6d78  73 bytes
D_NEWGAME       ends
D_TITLESEQ      segment word public 'DATA'     ; 6dc2  15 bytes
D_TITLESEQ      ends
D_COPYPROT      segment word public 'DATA'     ; 6dd2  1188 bytes
D_COPYPROT      ends
D_CREATE        segment word public 'DATA'     ; 7276  531 bytes
D_CREATE        ends
D_MODIFY        segment word public 'DATA'     ; 748a  112 bytes
D_MODIFY        ends
D_TRAIN         segment word public 'DATA'     ; 74fa  176 bytes
D_TRAIN         ends
D_ECLCMDS       segment word public 'DATA'     ; 75aa  797 bytes
D_ECLCMDS       ends
D_MOVE          segment word public 'DATA'     ; 78c8  718 bytes
D_MOVE          ends
D_CAMP          segment word public 'DATA'     ; 7b96  514 bytes
D_CAMP          ends
D_PARTY         segment word public 'DATA'     ; 7d98  444 bytes
D_PARTY         ends
D_MONEY         segment word public 'DATA'     ; 7f54  94 bytes
D_MONEY         ends
D_POSTCMBT      segment word public 'DATA'     ; 7fb2  163 bytes
D_POSTCMBT      ends
D_SHOP          segment word public 'DATA'     ; 8056  590 bytes
D_SHOP          ends
D_TEMPLE        segment word public 'DATA'     ; 82a4  95 bytes
D_TEMPLE        ends
D_EFFECTS       segment word public 'DATA'     ; 8304  856 bytes
D_EFFECTS       ends
D_FXCOMBAT      segment word public 'DATA'     ; 865c  300 bytes
D_FXCOMBAT      ends
D_SPELLFX       segment word public 'DATA'     ; 8788  934 bytes
D_SPELLFX       ends
D_SPELLCST      segment word public 'DATA'     ; 8b2e  579 bytes
D_SPELLCST      ends
D_CHARVIEW      segment word public 'DATA'     ; 8d72  929 bytes
D_CHARVIEW      ends
D_VIEW3D        segment word public 'DATA'     ; 9114  133 bytes
D_VIEW3D        ends
D_SYSUTIL       segment word public 'DATA'     ; 919a  17 bytes
D_SYSUTIL       ends
D_LINEEDIT      segment word public 'DATA'     ; 91ac  14 bytes
D_LINEEDIT      ends
D_PROMPT        segment word public 'DATA'     ; 91ba  38 bytes
D_PROMPT        ends
D_DISKFILE      segment word public 'DATA'     ; 91e0  136 bytes
D_DISKFILE      ends
D_DISPINIT      segment word public 'DATA'     ; 9268  293 bytes
D_DISPINIT      ends
D_VIDMOUSE      segment word public 'DATA'     ; 938e  1772 bytes
D_VIDMOUSE      ends
D_LOWIO         segment word public 'DATA'     ; 9a7a  57 bytes
D_LOWIO         ends
D_SCRFONT       segment word public 'DATA'     ; 9ab4  8 bytes
D_SCRFONT       ends
D_SPEAKER       segment word public 'DATA'     ; 9abc  48 bytes
D_SPEAKER       ends
D_PALETTE       segment word public 'DATA'     ; 9aec  21 bytes
D_PALETTE       ends
D_GLYPH         segment word public 'DATA'     ; 9b02  692 bytes
D_GLYPH         ends
D_GLYPHFX       segment word public 'DATA'     ; 9db6  87 bytes
D_GLYPHFX       ends
D_FCACHE        segment word public 'DATA'     ; 9e0e  369 bytes
D_FCACHE        ends
D_DIGISND       segment word public 'DATA'     ; 9f80  1 bytes
D_DIGISND       ends
D_LIBRARY       segment word public 'DATA'     ; 9f82  455 bytes
D_LIBRARY       ends
D_LIBEDIT       segment word public 'DATA'     ; a14a  108 bytes
D_LIBEDIT       ends
D_DIALOG        segment word public 'DATA'     ; a1b6  144 bytes
D_DIALOG        ends
D_LOWLEVEL      segment word public 'DATA'     ; a246  1 bytes
D_LOWLEVEL      ends
D_PATHFIND      segment word public 'DATA'     ; a248  14 bytes
D_PATHFIND      ends
D_PATHUTIL      segment word public 'DATA'     ; a256  1 bytes
D_PATHUTIL      ends
D_DIRFIND       segment word public 'DATA'     ; a258  4 bytes
D_DIRFIND       ends
D_FARMEM        segment word public 'DATA'     ; a25c  4 bytes
D_FARMEM        ends
_BSS            segment word public 'BSS'      ; a260  start of the zeroed region
bss_start       label   byte
_BSS            ends
B_REST          segment word public 'BSS'      ; a260  53 bytes
B_REST          ends
B_GAP_A295      segment byte public 'BSS'      ; a295  21 bytes
B_GAP_A295      ends
B_HERO          segment word public 'BSS'      ; a2aa  3 bytes
B_HERO          ends
B_TILES         segment word public 'BSS'      ; a2ae  892 bytes
B_TILES         ends
B_TEXTWIN       segment word public 'BSS'      ; a62a  42 bytes
B_TEXTWIN       ends
B_LISTBOX       segment word public 'BSS'      ; a654  4 bytes
B_LISTBOX       ends
B_MENU          segment word public 'BSS'      ; a658  353 bytes
B_MENU          ends
B_STRUTIL       segment word public 'BSS'      ; a7ba  92 bytes
B_STRUTIL       ends
B_SOUND         segment word public 'BSS'      ; a816  48 bytes
B_SOUND         ends
B_SFXCTL        segment word public 'BSS'      ; a846  2 bytes
B_SFXCTL        ends
B_CHARPIC       segment word public 'BSS'      ; a848  38 bytes
B_CHARPIC       ends
B_MISC          segment word public 'BSS'      ; a86e  35 bytes
B_MISC          ends
B_FRAME         segment word public 'BSS'      ; a892  4 bytes
B_FRAME         ends
B_CBACTION      segment word public 'BSS'      ; a896  196 bytes
B_CBACTION      ends
B_CMBSETUP      segment word public 'BSS'      ; a95a  562 bytes
B_CMBSETUP      ends
B_AITURN        segment word public 'BSS'      ; ab8c  288 bytes
B_AITURN        ends
B_GAP_ACAC      segment byte public 'BSS'      ; acac  4 bytes
B_GAP_ACAC      ends
B_GLBDATA       segment word public 'BSS'      ; acb0  14 bytes
B_GLBDATA       ends
B_ECLRUN        segment word public 'BSS'      ; acbe  8 bytes
B_ECLRUN        ends
B_SAVELOAD      segment word public 'BSS'      ; acc6  11 bytes
B_SAVELOAD      ends
B_PICVIEW       segment word public 'BSS'      ; acd2  1 bytes
B_PICVIEW       ends
B_POOLFILE      segment word public 'BSS'      ; acd4  2 bytes
B_POOLFILE      ends
B_GLOBALS1      segment word public 'BSS'      ; acd6  4630 bytes
B_GLOBALS1      ends
B_EFFECTS       segment word public 'BSS'      ; beec  916 bytes
B_EFFECTS       ends
B_GLOBALS2      segment word public 'BSS'      ; c280  1144 bytes
B_GLOBALS2      ends
B_COPYPROT      segment word public 'BSS'      ; c6f8  86 bytes
B_COPYPROT      ends
B_CREATE        segment word public 'BSS'      ; c74e  65 bytes
B_CREATE        ends
B_MODIFY        segment word public 'BSS'      ; c790  54 bytes
B_MODIFY        ends
B_ECLCMDS       segment word public 'BSS'      ; c7c6  18 bytes
B_ECLCMDS       ends
B_PARTY         segment word public 'BSS'      ; c7d8  6 bytes
B_PARTY         ends
B_POSTCMBT      segment word public 'BSS'      ; c7de  3 bytes
B_POSTCMBT      ends
B_SHOP          segment word public 'BSS'      ; c7e2  1 bytes
B_SHOP          ends
B_SPELLFX       segment word public 'BSS'      ; c7e4  2 bytes
B_SPELLFX       ends
B_SPELLCST      segment word public 'BSS'      ; c7e6  513 bytes
B_SPELLCST      ends
B_CHARVIEW      segment word public 'BSS'      ; c9e8  8 bytes
B_CHARVIEW      ends
B_VIEW3D        segment word public 'BSS'      ; c9f0  30 bytes
B_VIEW3D        ends
B_SYSUTIL       segment word public 'BSS'      ; ca0e  11 bytes
B_SYSUTIL       ends
B_PROMPT        segment word public 'BSS'      ; ca1a  2 bytes
B_PROMPT        ends
B_DISKFILE      segment word public 'BSS'      ; ca1c  11 bytes
B_DISKFILE      ends
B_VIDMOUSE      segment word public 'BSS'      ; ca28  893 bytes
B_VIDMOUSE      ends
B_LOWIO         segment word public 'BSS'      ; cda6  25 bytes
B_LOWIO         ends
B_SPEAKER       segment word public 'BSS'      ; cdc0  3 bytes
B_SPEAKER       ends
B_PALETTE       segment word public 'BSS'      ; cdc4  1773 bytes
B_PALETTE       ends
B_GLYPH         segment word public 'BSS'      ; d4b2  520 bytes
B_GLYPH         ends
B_FCACHE        segment word public 'BSS'      ; d6ba  980 bytes
B_FCACHE        ends
B_DIGISND       segment word public 'BSS'      ; da8e  1 bytes
B_DIGISND       ends
B_LIBRARY       segment word public 'BSS'      ; da90  44 bytes
B_LIBRARY       ends
B_GAP_DABC      segment byte public 'BSS'      ; dabc  4 bytes
B_GAP_DABC      ends
B_DIALOG        segment word public 'BSS'      ; dac0  18 bytes
B_DIALOG        ends
B_SFXSLOTS      segment word public 'BSS'      ; dad2  77 bytes
B_SFXSLOTS      ends
B_PATHFIND      segment word public 'BSS'      ; db20  1 bytes
B_PATHFIND      ends
B_DIRFIND       segment word public 'BSS'      ; db22  57 bytes
B_DIRFIND       ends
B_FMTOUT        segment word public 'BSS'      ; db5c  4 bytes
B_FMTOUT        ends
