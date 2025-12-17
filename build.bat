if not exist build mkdir build

arm-none-eabi-as.exe -mcpu=cortex-m0plus -mthumb src\start.s -o build\start.o
arm-none-eabi-as.exe -mcpu=cortex-m0plus -mthumb src\main.s  -o build\main.o

arm-none-eabi-ld.exe -T linker.ld build/start.o build/main.o -o firmware.elf

arm-none-eabi-readelf.exe -h firmware.elf > build\elfdata.txt
arm-none-eabi-objdump.exe -d firmware.elf > build\objdata.txt

elf2uf2.exe -i firmware.elf -f 0xe48bff56 -o build\firmware.uf2

del build\*.o firmware.elf

copy build\firmware.uf2 F:\