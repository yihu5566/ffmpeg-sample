cmake_minimum_required(VERSION 3.23)
project(ffmpeg_advanced_api C)

set(CMAKE_C_STANDARD 90)
include_directories(/usr/local/opt/ffmpeg@4/include)
link_directories(/usr/local/opt/ffmpeg@4/lib)
add_executable(ffmpeg_advanced_api main.c)
#add_executable(ffmpeg_advanced_api main_write.c)
target_link_libraries(ffmpeg_advanced_api
        avcodec
        avdevice
        avfilter
        avformat
        swscale
        swresample
        avutil

        )