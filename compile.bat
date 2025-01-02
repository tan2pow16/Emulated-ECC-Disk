@echo off
setlocal

set _RETURN_DIR=%CD%
cd /D %~dp0

gcc -c -Os -o cache\berlekamp.o rscode\berlekamp.c
gcc -c -Os -o cache\crcgen.o rscode\crcgen.c
gcc -c -Os -o cache\galois.o rscode\galois.c
gcc -c -Os -o cache\rs.o rscode\rs.c

gcc -c -Os -o cache\blk_ecc.o blk_ecc.c
gcc -c -Os -o cache\disk_util.o disk_util.c
gcc -c -Os -o cache\spd_proc.o spd_proc.c
gcc -c -Os -o cache\custom_test.o custom_test.c
gcc -c -Os -o cache\main.o main.c

gcc -o main.exe cache\berlekamp.o cache\crcgen.o cache\galois.o cache\rs.o cache\blk_ecc.o cache\disk_util.o cache\spd_proc.o cache\custom_test.o cache\main.o winspd\winspd_x64.dll -lsetupapi -lrpcrt4 -s

gcc -c -Os -o cache\test.o test\test.c
gcc -o test.exe cache\berlekamp.o cache\crcgen.o cache\galois.o cache\rs.o cache\blk_ecc.o cache\test.o -s

cd /D %_RETURN_DIR%

endlocal
