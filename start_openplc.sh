#!/bin/sh
if [ $# -eq 0 ]; then
    echo "Error: You must provide a ST file to be compiled as argument"
    exit 1
fi

# Select hardware layer
echo "Copying hardware layer..."
cp ./OpenPLC_v3/webserver/core/hardware_layers/engine.cpp ./OpenPLC_v3/webserver/core/hardware_layer.cpp
echo "Copying ST file..."
cp "$1" ./OpenPLC_v3/webserver/st_files/st_file.st

# Compile
echo "Compiling..."
cd OpenPLC_v3/webserver
bash ./scripts/compile_program.sh st_file.st
cd ../../

# Start OpenPLC
echo "Starting application..."
./OpenPLC_v3/webserver/core/openplc &
cd OpenPLC_v3/webserver
python2.7 webserver.py