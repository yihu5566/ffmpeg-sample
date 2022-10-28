cmake_minimum_required(VERSION 3.23)
project(ffmpeg_filter C)

set(CMAKE_C_STANDARD 90)
include_directories(/usr/local/Cellar/ffmpeg/5.1.2/include/)
link_directories(/usr/local/Cellar/ffmpeg/5.1.2/lib/)
add_executable(ffmpeg_filter main.c)
target_link_libraries(ffmpeg_filter
        avcodec
        avdevice
        avfilter
        avformat

        )
