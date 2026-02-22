cmake_minimum_required(VERSION 3.22.1)
project(ffmpeg_player)

add_library(ffmpeg_player SHARED ffmpeg_player.cpp)

find_library(log-lib log)
find_library(android-lib android)

# Expected prebuilt FFmpeg libs under app/src/main/cpp/ffmpeg/<abi>/
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/ffmpeg/${ANDROID_ABI})

add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavcodec.so)
add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavformat.so)
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavutil.so)
add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswresample.so)
add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswscale.so)

include_directories(${CMAKE_SOURCE_DIR}/ffmpeg/include)

target_link_libraries(
    ffmpeg_player
    avcodec
    avformat
    avutil
    swresample
    swscale
    ${android-lib}
    ${log-lib}
)
