#！/bin/sh
devmem 0x43c20128 32 0x4e20
i2cset -y -f 0 0x4a 0x14 0xff
i2cset -y -f 0 0x4a 0x15 0xff
i2cset -y -f 0 0x4a 0x1c 0x34
sender -d adc -f /run/media/mmcblk1p1/etc/V2_ADC_NCO2_3G.json
sender -d dac -f /run/media/mmcblk1p1/etc/V2_DAC_NCO2_3G.json
