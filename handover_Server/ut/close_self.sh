#！/bin/sh
devmem 0x43c20128 32 0x186A0
i2cset -y -f 0 0x4a 0x14 0x00
i2cset -y -f 0 0x4a 0x15 0x00
i2cset -y -f 0 0x4a 0x1c 0x34
sender -d adc -f /run/media/mmcblk1p1/etc/V2_ADC_NCO800M.json
sender -d dac -f /run/media/mmcblk1p1/etc/V2_DAC_NCO3_2G.json
