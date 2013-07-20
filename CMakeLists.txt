project(mediabarcode C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(Threads REQUIRED)

include_directories(include)

add_library(mediabarcode SHARED src/mediabarcode.c)
target_link_libraries(mediabarcode ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(mediabarcode-bin src/mediabarcode-bin.c)
set_target_properties(mediabarcode-bin PROPERTIES OUTPUT_NAME mediabarcode)
target_link_libraries(mediabarcode-bin mediabarcode)

add_executable(test src/test.c)
target_link_libraries(test mediabarcode)

CONFIGURE_FILE(res/mediabarcode.pc.in res/mediabarcode.pc @ONLY)

install(TARGETS mediabarcode DESTINATION lib)
install(TARGETS mediabarcode-bin DESTINATION bin)
install(FILES src/mediabarcode.h DESTINATION include)
install(FILES res/mediabarcode.pc DESTINATION lib/pkgconfig)
