#！/bin/sh
f=./arm
b=./build
# cjson
cd ./dependence
cd ./cjson
if [ -d $f ]
then 
    rm -rf $f
fi

if [ -d $b ]
then 
    rm -rf $b
fi

# gw_threadpool
cd ../gw_threadpool
if [ -d $f ]
then 
    rm -rf $f
fi

if [ -d $b ]
then 
    rm -rf $b
fi

# gw_msg_queue
cd ../gw_msg_queue
if [ -d $f ]
then 
    rm -rf $f
fi

if [ -d $b ]
then 
    rm -rf $b
fi

# gw_regdev
cd ../gw_regdev
if [ -d $f ]
then 
    rm -rf $f
fi

# gw_utility
cd ../gw_utility
if [ -d $f ]
then 
    rm -rf $f
fi

if [ -d $b ]
then 
    rm -rf $b
fi

# gw_management_frame
cd ../gw_management_frame
if [ -d $f ]
then 
    rm -rf $f
fi

# gw_control
cd ../gw_control
if [ -d $f ]
then 
    rm -rf $f
fi

# gw_tunnel
cd ../gw_tunnel
if [ -d "./build" ]
then 
    rm -rf ./build
fi

# gw_ipc
cd ../gw_ipc
if [ -d $f ]
then 
    rm -rf $f
fi

cd ../../BaseStation
if [ -d $f ]
then 
    rm -rf $f
fi

cd ../vehicle
if [ -d $f ]
then 
    rm -rf $f
fi

cd ../handoverServer
if [ -d "./build" ]
then 
    rm -rf ./build
fi

if [ -d $f ]
then 
    rm -rf $f
fi

# delete header and lib
cd ../dependence/include
find -L ./ -maxdepth 1 ! -name "broker.h" ! -name "gw_macros_util.h" ! -name "cst_net.h" ! -wholename "./" -exec rm -rf {} \;

cd ../lib
rm -rf *

cd ../lib_arm
find -L ./ -maxdepth 1 ! -name "libbroker.so" ! -name "libmosquitto.so" ! -name "libcst.so" ! -wholename "./" -exec rm -rf {} \;

# gwapplib
cd ..
if [ -d $f ]
then 
    rm -rf $f
fi







