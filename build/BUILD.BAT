@echo off
rem BUILD.BAT -- compile, assemble and link DQK.EXE.  Run by build/build.py with
rem C: = the staged tree (SRC, BUILD, OUT), D: = Turbo C 2.01, E: = TASM 3.1,
rem F: = RTLink/Plus.
set PATH=D:\;E:\;F:\
cd \SRC

echo Compiling > \OUT\CC.LOG
for %%f in (*.C) do tcc -c -ml -O -Z -d %%f >> \OUT\CC.LOG

echo Assembling > \OUT\AS.LOG
tasm /ml /dLINKED START.ASM >> \OUT\AS.LOG
tasm /m /w+ /ml AIL.ASM >> \OUT\AS.LOG
for %%f in (LOWLEVEL GFXUTIL DISSOLVE PCMPLAY PIXROW VGADRAW LZCODEC) do tasm /ml %%f.ASM >> \OUT\AS.LOG
for %%f in (CONIO CPUTS STRCMP STRINGS DOSIO MEMOPS FARMEM SYSINT BIOS) do tasm /ml %%f.ASM >> \OUT\AS.LOG
for %%f in (LDIV LXMUL LLSH LRSH LURSH SCOPY ENVGET GLOBALS) do tasm /ml %%f.ASM >> \OUT\AS.LOG
rem RTLERR and RTLFIND reconstruct the shipped RTLink/Plus run-time; the linker
rem supplies its own with its overlay manager, so they are assembled but not linked.
for %%f in (RTLERR RTLFIND) do tasm /ml %%f.ASM >> \OUT\AS.LOG

copy \BUILD\DQK.LNK . > NUL
copy \BUILD\RTLINK.CFG . > NUL
rtlink @DQK.LNK > \OUT\LINK.LOG
if exist DQK.EXE copy DQK.EXE \OUT > NUL
if exist DQK.MAP copy DQK.MAP \OUT > NUL
