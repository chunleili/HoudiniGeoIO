set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(HoudiniGeoIO)
set(HoudiniGeoIO_SOURCES
    HoudiniGeoIO.cpp
    HoudiniGeoIO.h
)
set(HoudiniGeoIO_HEADERS
    HoudiniGeoIO.h
)
# a test main
add_executable(HoudiniGeoIO  main ${HoudiniGeoIO_SOURCES} ${HoudiniGeoIO_HEADERS})