cmake_minimum_required(VERSION 3.5)

set(CXX_STANDARD 11)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

project(libdbusserver)

add_executable(libdbusserver main.cpp)

find_path(dbusDir dbus.h)
message(${dbusDir})
include_directories(/usr/include/dbus-1.0
                    /usr/lib/x86_64-linux-gnu/dbus-1.0/include
                    #${dbusDir}
                    /software/sharedlibrary/json/include
                    )

target_link_libraries(libdbusserver dbus-1 pthread)
