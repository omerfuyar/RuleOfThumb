@echo off

if not exist build mkdir build

echo [1/6] Assembling start.s...
arm-none-eabi-as.exe -mcpu=cortex-m0plus -mthumb src\start.s -o build\start.o || goto :error

echo [2/6] Assembling main.s...
arm-none-eabi-as.exe -mcpu=cortex-m0plus -mthumb src\main.s  -o build\main.o || goto :error

echo [3/6] Linking...
arm-none-eabi-ld.exe -T linker.ld build/start.o build/main.o -o firmware.elf || goto :error

echo [4/6] Generating ELF data...
arm-none-eabi-readelf.exe -h firmware.elf > build\elfdata.txt || goto :error
arm-none-eabi-objdump.exe -d firmware.elf > build\objdata.txt || goto :error

echo [5/6] Converting to UF2...
elf2uf2.exe -i firmware.elf -f 0xe48bff56 -o build\firmware.uf2 || goto :error

echo Cleaning up...
del build\*.o firmware.elf

echo ----------------
echo Build Successful
echo ----------------

echo [6/6] Copying to device...
copy build\firmware.uf2 F:\ || goto :errorFlash

echo ----------------
echo Flash Successful!
echo ----------------
exit /b 0

:error
echo ----------------
echo Build Failed!
echo ----------------
exit /b 1

:errorFlash
echo ----------------
echo Flash Failed!
echo ----------------
exit /b 2