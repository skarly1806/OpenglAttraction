#!/bin/bash
sudo rm -r build
mkdir build
cd build/
sudo cmake ../
sudo make
cd ..