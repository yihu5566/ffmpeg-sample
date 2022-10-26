cmake_minimum_required(VERSION 3.23)
project(ffmpeg_demo C)

set(CMAKE_C_STANDARD 99)

include_directories(/usr/local/Cellar/ffmpeg/5.1.2/include/)
link_directories(/usr/local/Cellar/ffmpeg/5.1.2/lib/)

add_executable(ffmpeg_demo main.c)

target_link_libraries(ffmpeg_demo
        avcodec
        avdevice
        avfilter
        avformat

        )

