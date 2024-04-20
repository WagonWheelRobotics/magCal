#!/bin/bash

#
# CERES with SuiteSparse
#

common_path=$(pwd)/../common

install_path=$(pwd)/mingw73_64
tp_ceres_path=$(pwd)
source $common_path/mingw_build_env
echo $install_path

#rm -rf $install_path
mkdir -p $install_path

lapack_check=$install_path/lib/liblapack.dll.a
eigen3_check=$install_path/include/eigen3/Eigen/Eigen
gflags_check=$install_path/lib/libgflags.dll.a
glog_check=$install_path/lib/libglog.dll.a
sparse_check=$install_path/lib/SuiteSparse/libcholmod.dll.a
ceres_check=$install_path/lib/libceres.dll.a


export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$install_path/share/pkgconfig:$install_path/lib/pkgconfig

mkdir -p ../../temp
cd ../../temp

if [ -f "cmake-3.22.6/install/bin/cmake" ]; then
    echo "cmake 3.22.6 exists."
    export PATH=$(pwd)/cmake-3.22.6/install/bin:$PATH
else
    rm -rf cmake-3.22.6
    if [ -f "cmake-3.22.6.tar.gz" ]; then
        echo "cmake-3.22.6.tar.gz exists."
    else
        wget https://cmake.org/files/v3.22/cmake-3.22.6.tar.gz
    fi
    tar xvf cmake-3.22.6.tar.gz
    pushd cmake-3.22.6
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" .. -DCMAKE_USE_OPENSSL=OFF -DCMAKE_INSTALL_PREFIX=$(pwd)/../install
    cmake --build . -j 8
    cmake --install .
    export PATH=$(pwd)/../install/bin:$PATH
    popd
fi


if [ -f $lapack_check ]; then
    echo "lapack exists."
else
    rm -rf lapack-release
    git clone https://github.com/Reference-LAPACK/lapack-release.git -b lapack-3.10.0
    pushd lapack-release
    mkdir build
    cd build 
    cmake -G "MinGW Makefiles" ../ -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$install_path
    cmake --build . -j 8
    cmake --install .
    popd
    if [ -f $lapack_check ]; then
        echo "lapack installed."
    else
        echo "lapack install failed."
        exit
    fi
fi

if [ -f $eigen3_check ]; then 
    echo "eigen3 exists." 
else
    rm -rf eigen-3.3.9
    if [ -f "eigen-3.3.9.tar.gz" ]; then
        echo "eigen-3.3.9.tar.gz exists."
    else
        wget https://gitlab.com/libeigen/eigen/-/archive/3.3.9/eigen-3.3.9.tar.gz
    fi
    tar xvf eigen-3.3.9.tar.gz
    pushd eigen-3.3.9
    mkdir build
    cmake -G "MinGW Makefiles" -S . -B ./build/ -DCMAKE_INSTALL_PREFIX:PATH=$install_path
    cmake --install ./build
    popd

    if [ -f $eigen3_check ]; then 
        echo "eigen3 installed" 
    else
        echo "eigen3 install failed" 
        exit
    fi
fi


if [ -f $gflags_check ]; then 
    echo "gflags exists." 
else
    rm -rf gflags
    git clone https://github.com/gflags/gflags.git -b v2.2.2
    pushd gflags
    mkdir gflags_build
    cd gflags_build
    cmake -G "MinGW Makefiles" ../ -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=$install_path
    cmake --build . -j 8
    cmake --install .
    popd
    if [ -f $gflags_check ]; then 
        echo "gflags installed." 
    else
        echo "gflags install failed." 
        exit
    fi
fi


if [ -f $glog_check ]; then 
    echo "glog exists." 
else
    rm -rf glog
    git clone https://github.com/google/glog.git -b v0.6.0
    pushd glog
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" ../ -DBUILD_SHARED_LIBS=ON -Dgflags_DIR=$install_path/lib/cmake/gflags -DCMAKE_INSTALL_PREFIX=$install_path
    cmake --build . -j 8
    cmake --install .
    popd
    if [ -f $glog_check ]; then 
        echo "glog installed."
    else
        echo "glog install failed."
        exit
    fi
fi


if [ -f $sparse_check ]; then 
    echo "SuiteSparse exists." 
else
    rm -rf SuiteSparse
    git clone https://github.com/sergiud/SuiteSparse.git
    pushd SuiteSparse
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" ../ -DBUILD_SHARED_LIBS=ON -DWITH_DEMOS=OFF -DCMAKE_INSTALL_PREFIX=$install_path
    cmake --build . -j 8
    cmake --install .
    popd
    if [ -f $sparse_check ]; then 
        echo "SuiteSparse installed." 
    else
        echo "SuiteSparse install failed." 
        exit
    fi
fi


if [ -f $ceres_check ]; then 
    echo "ceres-solver exists." 
else
    rm -rf ceres-solver
    git clone https://ceres-solver.googlesource.com/ceres-solver -b 2.2.0
    pushd ceres-solver
    mkdir ceres_build
    cd ceres_build
    cmake -G "MinGW Makefiles" ../ -DBUILD_SHARED_LIBS=ON -DLAPACK_LIBRARIES=$install_path/lib/liblapack.dll.a -DBLAS_LIBRARIES=$install_path/lib/libblas.dll.a -DCXSparse_DIR=$install_path/lib/CXSparse/cmake -DSuiteSparse_DIR=$install_path/lib/SuiteSparse/cmake -Dgflags_DIR=$install_path/lib/cmake/gflags -Dglog_DIR=$install_path/lib/cmake/glog -DCMAKE_INSTALL_PREFIX=$install_path
    cp $tp_ceres_path/liborder.py ./liborder.py
    python3 ./liborder.py
    cmake --build . -j 8
    cmake --install .
    popd
    if [ -f $ceres_check ]; then 
        echo "ceres-solver installed." 
    else
        echo "ceres-solver install failed." 
        exit
    fi
fi

#
# done
#

