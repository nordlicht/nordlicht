project(mediabarcode C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(Threads REQUIRED)

include_directories(include)

add_library(mediabarcode SHARED src/mediabarcode.c)
target_link_libraries(mediabarcode ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(mbc bin/mbc.c)
target_link_libraries(mbc mediabarcode)

add_executable(test-mediabarcode test/test.c)
target_link_libraries(test-mediabarcode mediabarcode)

CONFIGURE_FILE(mediabarcode.pc.in mediabarcode.pc @ONLY)

install(TARGETS mediabarcode DESTINATION lib)
install(TARGETS mbc DESTINATION bin)
install(FILES include/mediabarcode.h DESTINATION include)
install(FILES mediabarcode.pc DESTINATION lib/pkgconfig)
