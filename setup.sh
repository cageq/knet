#!/bin/bash 
curPath=`pwd`

#depsPath=`realpath $curPath/deps`
depsPath=${curPath}/deps
mkdir -p ${depsPath}/lib
echo "deps path is" $depsPath
echo "SET(WORKROOT $curPath )" > env.cmake 

LIBFMTVER=fmt-7.0.3
unzip -o opt/${LIBFMTVER}.zip -d opt 
pushd . 
cd opt/${LIBFMTVER}
#cmake -DCMAKE_INSTALL_PREFIX=${depsPath}  . 
cmake -DCMAKE_INSTALL_PREFIX=${depsPath} -DCMAKE_CXX_FLAGS=-fPIC . 
make -j4 && make install 
popd 


#LIBGO=libgo-2.6
#tar  zxvf opt/${LIBGO}.tar.gz  --directory=$curPath/opt    
#pushd . 
#cd opt/${LIBGO}
#cmake -DCMAKE_INSTALL_PREFIX=${depsPath}  . 
#make -j4 && make install 
#popd 
#

LIBASIOVER=asio-1.18.0
tar  zxvf opt/$LIBASIOVER.tar.gz --directory=$curPath/opt    
mkdir -p ${depsPath}/include/
/bin/cp -R opt/${LIBASIOVER}/include/*  ${depsPath}/include/

echo 'include_directories("${WORKROOT}/deps/include")' >> env.cmake 
echo 'link_directories("${WORKROOT}/deps/lib")' >> env.cmake 
echo 'link_directories("${WORKROOT}/deps/lib64")' >> env.cmake 



