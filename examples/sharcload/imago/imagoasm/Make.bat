@echo off
del *.obj
del *.exe
del *.lst
del *.map
del *.ldr
asm21k -adsp21060 -v -l imago.asm
PAUSE
echo             !!!!!!!!!!!!!!!!!!   LINKER  PROGRAM   !!!!!!!!!!!!!!!              
ld21k -m -a ..\ArchFile\arc_a061.ach imago
PAUSE
echo             !!!!!!!!!!   CREATE LOADER-FILE  PROGRAM   !!!!!!!!!!!
ldr21k imago.exe -blink -fascii -l ..\060_link\060_link.exe -o imago.ldr

