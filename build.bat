make
arm-none-eabi-strip lpp-3ds.elf
makerom2 -f cci -o lpp-3ds.3ds -rsf gw_workaround.rsf -target d -exefslogo -elf lpp-3ds.elf -icon icn_sun.bin -banner bnr_sun.bin
makerom -f cia -o cia_workaround.cia -elf lpp-3ds.elf -rsf sunshell_cia.rsf -icon icn_sun.bin -banner bnr_sun.bin -exefslogo -target t 