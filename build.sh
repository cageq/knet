#!/bin/bash 


cd opt 
tar zxf spdlog-1.11.0.tar.gz
tar jxf asio-1.24.0.tar.bz2
unzip -o fmt-9.1.0.zip
tar zxvf llhttp-v6.0.5.tgz

cd ..

cmake . 
make -j4
