linux{

CERES=$$PWD/linux
EIGEN=$$PWD/../eigen/linux
INCLUDEPATH += $${CERES}/include \
$${EIGEN}/include/eigen3

LIBS += \
$${CERES}/lib/libglog.a \
$${CERES}/lib/libgflags.a \
$${CERES}/lib/libceres.a \
$${CERES}/lib/SuiteSparse/libspqr.a \
$${CERES}/lib/SuiteSparse/libcholmod.a \
$${CERES}/lib/SuiteSparse/libamd.a \
$${CERES}/lib/SuiteSparse/libcamd.a \
$${CERES}/lib/SuiteSparse/libccolamd.a \
$${CERES}/lib/SuiteSparse/libcolamd.a \
$${CERES}/lib/SuiteSparse/libsuitesparseconfig.a \
$${CERES}/lib/CXSparse/libcxsparse.a \
$${CERES}/lib/liblapack.a \
$${CERES}/lib/libblas.a \
-lgfortran \


DEFINES += GLOG_NO_ABBREVIATED_SEVERITIES
DEFINES += GOOGLE_GLOG_DLL_DECL
message(CERES linux:$${CERES})
DEFINES += CERES_EXISTS
}



win32-g++{

CERES=$$PWD/mingw73_64
INCLUDEPATH += \
$${CERES}/include \
$${CERES}/include/eigen3

LIBS += \
$${CERES}/lib/libglog.dll.a \
$${CERES}/lib/libgflags.dll.a \
$${CERES}/lib/libceres.dll.a \
$${CERES}/lib/SuiteSparse/libspqr.dll.a \
$${CERES}/lib/SuiteSparse/libcholmod.dll.a \
$${CERES}/lib/SuiteSparse/libamd.dll.a \
$${CERES}/lib/SuiteSparse/libcamd.dll.a \
$${CERES}/lib/SuiteSparse/libccolamd.dll.a \
$${CERES}/lib/SuiteSparse/libcolamd.dll.a \
$${CERES}/lib/SuiteSparse/libsuitesparseconfig.dll.a \
$${CERES}/lib/CXSparse/libcxsparse.dll.a \
$${CERES}/lib/liblapack.dll.a \
$${CERES}/lib/libblas.dll.a \
-lgfortran \
-lshlwapi

LIBS += -L$${CERES}/bin

#$${CERES}/lib/libgflags_nothreads.dll.a \

DEFINES += GLOG_NO_ABBREVIATED_SEVERITIES
DEFINES += GOOGLE_GLOG_DLL_DECL
message(CERES win32-g++:$${CERES})
DEFINES += CERES_EXISTS
}

