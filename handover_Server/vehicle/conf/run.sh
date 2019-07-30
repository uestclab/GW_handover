#£¡/bin/sh
nohup ./vehicle 1>result.out 2>result.out &

while [ true ]; do
/bin/sleep 5
clear
cat result.out
done
