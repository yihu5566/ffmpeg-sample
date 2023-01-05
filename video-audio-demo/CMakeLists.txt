cmake_minimum_required(VERSION 3.23)
project(video_audio_demo C)

include_directories(/usr/local/opt/ffmpeg@4/include)
link_directories(/usr/local/opt/ffmpeg@4/lib)

set(CMAKE_C_STANDARD 90)

add_executable(video_audio_demo main.c)

target_link_libraries(video_audio_demo
        avcodec
        avfilter
        avformat
        avutil
        avdevice
        )
