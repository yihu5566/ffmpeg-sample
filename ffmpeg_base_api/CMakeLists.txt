cmake_minimum_required(VERSION 3.23)
project(ffmpeg_base_api C)

set(CMAKE_C_STANDARD 90)

include_directories(/usr/local/opt/ffmpeg@4/include)
link_directories(/usr/local/opt/ffmpeg@4/lib)

add_executable(ffmpeg_demo main.c)

target_link_libraries(ffmpeg_demo
        avcodec
        avdevice
        avfilter
        avformat

        )
