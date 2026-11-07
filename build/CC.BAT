@echo off
rem CC.BAT <module> -- compile one C module for the RTLink/Plus build.
rem -zR and -zD name its _DATA D_<module> and its _BSS B_<module>, so that
rem start.asm can place them in the shipped order (dgorder.inc); the code is
rem unchanged.  -DLINKED selects the link-only declarations (config.c).
tcc -c -ml -O -Z -d -DLINKED -zRD_%1 -zDB_%1 %1.C >> \OUT\CC.LOG
