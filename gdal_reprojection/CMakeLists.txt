cmake_minimum_required(VERSION 3.10)
project(gdal_reprojection VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GDAL REQUIRED)

add_executable(gdal_reprojection main.cpp)

set(
    CMAKE_BUILD_TYPE 
    Debug
)

set(
    GDAL_DIR
    /usr
)
target_link_libraries(
    gdal_reprojection
    GDAL::GDAL
)
