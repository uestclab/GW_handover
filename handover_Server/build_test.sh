#！/bin/sh

echo call clean.sh
sh clean.sh


f=./arm
b=./build

# cjson
cd ./dependence
cd ./cjson
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi
cd ./arm
cmake ..
make && make install

cd ..
if [ -d $b ]
then 
    rm -rf $b
	mkdir $b
else
    mkdir $b
fi
cd ./build
cmake .. -DPUBLIC=OFF
make && make install

# gwapplib
cd ../../
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi
cd ./arm
cmake ..
make

cp ../gw_control/gw_control.h ../include
cp ../gw_management_frame/gw_frame.h ../include
cp ../gw_msg_queue/msg_queue.h ../include
cp ../gw_regdev/regdev_common.h ../include
cp ../gw_threadpool/SimpleQueue.h ../include
cp ../gw_threadpool/ThreadPool.h ../include
cp ../gw_utility/gw_utility.h ../include

# gw_tunnel
cd ../gw_tunnel
if [ -d "./build" ]
then 
    rm -rf ./build
	mkdir ./build
else
    mkdir ./build
fi
cd ./build
cmake ..
make && make install

# gw_utility
cd ../../gw_utility
if [ -d $b ]
then 
    rm -rf $b
	mkdir $b
else
    mkdir $b
fi
cd ./build
cmake .. -DPUBLIC=OFF
make && make install


cd ../../../BaseStation
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi
cd ./arm
cmake ..
make

cd ../../vehicle
if [ -d $f ]
then 
    rm -rf $f
	mkdir $f
else
    mkdir $f
fi
cd ./arm
cmake ..
make

cd ../../handoverServer
if [ -d "./build" ]
then 
    rm -rf ./build
	mkdir ./build
else
    mkdir ./build
fi
cd ./build
cmake ..
make

# display build result








