cmake_minimum_required(VERSION 3.20)
project(weed_box_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV is used by Python for camera display; the C++ UDP bridge
# only processes rectangle coordinates and needs Winsock on Windows.
add_executable(weed_box_parser
    main.cpp
    sectors.cpp
    spray_zone.cpp
)

target_link_libraries(weed_box_parser PRIVATE ws2_32)
