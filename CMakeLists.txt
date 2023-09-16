cmake_minimum_required(VERSION 3.26)
project(FFTest)

set(CMAKE_CXX_STANDARD 14)
set(FFMPEG_DIR /usr/local/Cellar/ffmpeg/6.0_1) # FFmpeg的安装目录，可以通过命令"brew info ffmpeg"获取
include_directories(${FFMPEG_DIR}/include/) # 头文件搜索路径
link_directories(${FFMPEG_DIR}/lib/) # 动态链接库或静态链接库的搜索路径

set(SDL_DIR  /usr/local/Cellar/sdl2/2.28.3)   #sdl目录
include_directories(${SDL_DIR}/include/) # 添加SDL头文件搜索路径
link_directories(${SDL_DIR}/lib/)

include_directories(FFTest)

add_executable(FFTest
        FFTest/avio_reading.c
        FFTest/decode_video.c
        FFTest/main.cpp
        FFTest/simplest_ffmpeg_picture_encoder.cpp
        FFTest/test_copy.cpp
        FFTest/test_copy.h
        FFTest/simple_player.cpp
)

# 添加链接库
target_link_libraries(
        FFTest

        SDL2main
        SDL2

        avcodec
        avdevice
        avfilter
        avformat
        swscale
        swresample
)