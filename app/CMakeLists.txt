#指定cmake版本
cmake_minimum_required(VERSION 3.4.1)
#设置生成的so动态库最后输出的路径
ADD_LIBRARY(ClientFtp
            SHARED
            src/main/cpp/clientFtp.cpp)

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

target_link_libraries(ClientFtp
                      ${log-lib})
