
# KNet 
Simple morden c++ network library wrapper based on asio standalone version , provide simple apis to write network applications. 



## Build 
the library contains a simple log library to print debug log. using fmt library to accelerate the log output.

you can use "setup.sh" to install dependence lib into deps directory.   

the library is headonly library, basically you can copy to you project and use it.


## build samples

./setup.sh   # install asio fmt lib to deps directory, no need root permissions. 
cmake . 
make -j4 


