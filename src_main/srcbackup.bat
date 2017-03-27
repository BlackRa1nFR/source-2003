
@echo off
rem Backs up all source to wherever you specify.

pkzip25 -add -max -dir=full -rec %1 *.txt *.mak *.cpp *.h *.c *.h *.dsp *.rc *.rc2 *.bat *.ico *.def *.doc *.s *.inl %3 %4 %5 %6

