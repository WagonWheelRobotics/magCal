# magCal

3 axis magnetometer calibration

## Platform
Windows (Qt 5.12.12 Mingw64)

## Third party libraries
1. QCustomPlot (GPL)
    - to build this library, run thirdParty/QCustomPlot/build_****.sh
2. Celes solver
    - to build this library, run thirdParty/ceres/build_****.sh
3. EDL module from Cloud compare (GPL)

build_mingw.sh uses mingw toolchain from Qt, script must run on MSYS2.
You need to install several packages using pacman (cmake, make, git etc)

