#!/bin/bash 


cd opt 
tar zxf spdlog-1.9.2.tar.gz
tar jxf asio-1.21.0.tar.bz2
unzip -o fmt-8.0.1.zip
tar zxvf llhttp-v6.0.5.tgz

cd ..

cmake . 
make -j4
