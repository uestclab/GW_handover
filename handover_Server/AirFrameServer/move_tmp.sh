#！/bin/sh
f=/mnt/hgfs/vm/tmp/share
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi

cp ./arm/airFrame /mnt/hgfs/vm/tmp/share/
cp ../ut/arm/ipc_client /mnt/hgfs/vm/tmp/share/ 
#cp ../dependence/lib_arm/libgw_ipc.so /mnt/hgfs/vm/tmp/share/
