#!/bin/sh

g++ --std=c++17 -osniffer sniffer.cpp -lpcap
./sniffer