cd libctru
make
cd ..
make
arm-none-eabi-strip lpp-3ds.elf
makerom2 -f cci -o lpp-3ds.3ds -rsf gw_workaround.rsf -target d -exefslogo -elf lpp-3ds.elf -icon icon.bin -banner banner.bin
makerom -f cia -o lpp-3ds.cia -elf lpp-3ds.elf -rsf cia_workaround.rsf -icon icon.bin -banner banner.bin -exefslogo -target t 