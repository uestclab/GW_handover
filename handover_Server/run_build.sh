#！/bin/sh
f=./arm
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

# gw_msg_queue
cd ../../gw_msg_queue
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

# gw_regdev
cd ../../gw_regdev
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

# gw_utility
cd ../../gw_utility
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

# gw_management_frame
cd ../../gw_management_frame
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

# gw_control
cd ../../gw_control
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

# gw_tunnel
cd ../../gw_tunnel
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

