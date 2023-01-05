cmake_minimum_required(VERSION 3.23)
project(video_demo C)

set(CMAKE_C_STANDARD 90)
include_directories(/usr/local/opt/ffmpeg@4/include)
link_directories(/usr/local/opt/ffmpeg@4/lib)
#add_executable(video_demo main.c)
#add_executable(video_demo extr_audio.c)
add_executable(video_demo remuxing.c)
#add_executable(video_demo merge_v_a.c)
target_link_libraries(video_demo
        avcodec
        avdevice
        avfilter
        avformat
        swscale
        swresample
        avutil

        )