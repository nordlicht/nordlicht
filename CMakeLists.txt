project(mediastrip C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(Threads REQUIRED)

include_directories(include)

add_library(mediastrip SHARED src/mediastrip.c)
target_link_libraries(mediastrip ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(mediastrip-bin src/mediastrip-bin.c)
set_target_properties(mediastrip-bin PROPERTIES OUTPUT_NAME mediastrip)
target_link_libraries(mediastrip-bin mediastrip)

add_executable(test src/test.c)
target_link_libraries(test mediastrip)

CONFIGURE_FILE(mediastrip.pc.in mediastrip.pc @ONLY)

install(TARGETS mediastrip DESTINATION lib)
install(TARGETS mediastrip-bin DESTINATION bin)
install(FILES src/mediastrip.h DESTINATION include)
install(FILES mediastrip.pc DESTINATION lib/pkgconfig)
