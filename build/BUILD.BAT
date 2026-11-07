@echo off
rem BUILD.BAT -- compile, assemble and link DQK.EXE.  Run by build/build.py with
rem C: = the staged tree (SRC, BUILD, OUT), D: = Turbo C 2.01, E: = TASM 3.1,
rem F: = RTLink/Plus.
set PATH=D:\;E:\;F:\
cd \SRC

echo Compiling > \OUT\CC.LOG
for %%m in (AITURN APPROACH CAMP CBACTION CBTUTIL CBVIEW CHARPIC CHARTAB) do call \BUILD\CC.BAT %%m
for %%m in (CHARVIEW CMBSETUP CMDARGS COMBAT COMBTAB CONFIG COPYPROT CREATE) do call \BUILD\CC.BAT %%m
for %%m in (CURSOR DIALOG DIGISND DIRFIND DIRNAMES DISKFILE DISPINIT ECLCMDS) do call \BUILD\CC.BAT %%m
for %%m in (ECLRUN EFFECTS FCACHE FMTOUT FRAME FXCOMBAT FXCORE GAMEMAIN) do call \BUILD\CC.BAT %%m
for %%m in (GAMETEXT GLBDATA GLYPH GLYPHFX HERO HEROSTAT INPGLUE LIBEDIT) do call \BUILD\CC.BAT %%m
for %%m in (LIBRARY LINEEDIT LISTBOX LOWIO MENU MISC MODIFY MONEY MOVE) do call \BUILD\CC.BAT %%m
for %%m in (NEWGAME NUMPARSE PALETTE PARTY PATHFIND PATHUTIL PICVIEW) do call \BUILD\CC.BAT %%m
for %%m in (POOLFILE POOLINIT POSTCMBT PROMPT REST ROSTER SAVELOAD SCRFONT) do call \BUILD\CC.BAT %%m
for %%m in (SCROLL SFXCTL SFXSLOTS SHOP SNDEVENT SOUND SPEAKER SPELLCST) do call \BUILD\CC.BAT %%m
for %%m in (SPELLFX SPELLTAB STARTDAT STRSCAN STRUTIL SYSUTIL TEMPLE TEXTFMT) do call \BUILD\CC.BAT %%m
for %%m in (TEXTWIN TILES TITLESEQ TRAIN VIDMOUSE VIEW3D) do call \BUILD\CC.BAT %%m

echo Assembling > \OUT\AS.LOG
rem /dLINKED selects the link-only parts of start, lowlevel and farmem; the
rem rest assemble as they do for the byte-exact build.
for %%f in (START LOWLEVEL FARMEM) do tasm /ml /dLINKED %%f.ASM >> \OUT\AS.LOG
tasm /m /w+ /ml AIL.ASM >> \OUT\AS.LOG
for %%f in (GFXUTIL DISSOLVE PCMPLAY PIXROW VGADRAW LZCODEC CONIO CPUTS) do tasm /ml %%f.ASM >> \OUT\AS.LOG
for %%f in (STRCMP STRINGS DOSIO MEMOPS SYSINT BIOS LDIV LXMUL LLSH LRSH) do tasm /ml %%f.ASM >> \OUT\AS.LOG
for %%f in (LURSH SCOPY ENVGET GLOBALS) do tasm /ml %%f.ASM >> \OUT\AS.LOG
rem RTLERR and RTLFIND reconstruct the shipped RTLink/Plus run-time; the linker
rem supplies its own with its overlay manager, so they are assembled but not linked.
for %%f in (RTLERR RTLFIND) do tasm /ml %%f.ASM >> \OUT\AS.LOG

copy \BUILD\DQK.LNK . > NUL
copy \BUILD\RTLINK.CFG . > NUL
rtlink @DQK.LNK > \OUT\LINK.LOG
if exist DQK.EXE copy DQK.EXE \OUT > NUL
if exist DQK.MAP copy DQK.MAP \OUT > NUL
