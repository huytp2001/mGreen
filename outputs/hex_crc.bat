.\outputs\srec_cat .\Application\Objects\MgreenNG.hex -intel -fill 0xFF -over .\Application\Objects\MgreenNG.hex -intel -o .\outputs\mgre.hex -intel -obs=16
.\outputs\srec_cat .\outputs\mgre.hex -intel -exclusive-maximum-l-e 0x0FFFF0 4 -crc32-l-e 0x0FFFF4 -o .\outputs\mgre.hex -intel -obs=16
.\outputs\hex2bin -s 0x10000 .\outputs\mgre.hex
