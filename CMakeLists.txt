project(vidcode C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(Threads REQUIRED)

add_library(vidcode SHARED src/vidcode.c)
target_link_libraries(vidcode ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(main bin/main.c)
target_link_libraries(main vidcode)

add_executable(test test/test.c)
target_link_libraries(test vidcode)

install(TARGETS vidcode DESTINATION lib)
install(FILES include/vidcode.h DESTINATION include)
install(FILES vidcode.pc DESTINATION lib/pkgconfig)
