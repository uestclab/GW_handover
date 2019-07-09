#！/bin/sh
f=./arm
# cjson
cd ./dependence
cd ./cjson
if [ -d $f ]
then 
    rm -rf $f
fi

# gw_msg_queue
cd ../gw_msg_queue
if [ -d $f ]
then 
    rm -rf $f
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

