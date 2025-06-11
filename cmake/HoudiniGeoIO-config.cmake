set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(HoudiniGeoIO_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../HoudiniGeoIO.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../HoudiniGeoIO.h
)
set(HoudiniGeoIO_INCLUDE_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../
)

# Usage:
# find_package(HoudiniGeoIO REQUIRED PATHS /path/to/HoudiniGeoIO)
# SET (YOUR_SRCS 
# 	${YOUR_SRCS}
# 	${HoudiniGeoIO_SRCS}
# )
# add_executable(YouApplication 
# 	${YOUR_SRCS}
# )
# target_include_directories(YouApplication PRIVATE ${HoudiniGeoIO_INCLUDE_DIR})
