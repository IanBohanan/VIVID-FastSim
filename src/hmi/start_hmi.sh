#!/bin/sh

g++ -std=gnu++17 -I /usr/local/boost_1_79_0 -I /usr/local/include/modbus -pthread ../parseConfig.cpp ../shmSetup.cpp ./hmi.cpp -L /usr/local/lib/ -lmodbus -o hmi -lrt
./hmi