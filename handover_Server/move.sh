#！/bin/sh
f=/mnt/hgfs/vm/tmp/handover
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi

cp ./BaseStation/arm/bs_main /mnt/hgfs/vm/tmp/handover/
cp ./vehicle/arm/vehicle /mnt/hgfs/vm/tmp/handover/

x=/mnt/hgfs/vm/tmp/lib_arm
if [ -d $x ]
then 
    rm -rf $x
	mkdir $x
else
    mkdir $x
fi

cp -r ./dependence/lib_arm/* /mnt/hgfs/vm/tmp/lib_arm/  

