project(nordlicht C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(Threads REQUIRED)

include_directories(include)

add_library(nordlicht SHARED src/nordlicht.c src/frame.c src/common.c)
target_link_libraries(nordlicht ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(nordlicht-tool src/nordlicht-tool.c)
set_target_properties(nordlicht-tool PROPERTIES OUTPUT_NAME nordlicht)
target_link_libraries(nordlicht-tool nordlicht)

configure_file(res/nordlicht.pc.in res/nordlicht.pc @ONLY)

install(TARGETS nordlicht DESTINATION lib)
install(TARGETS nordlicht-tool DESTINATION bin)
install(FILES src/nordlicht.h DESTINATION include)
install(FILES ${CMAKE_BINARY_DIR}/res/nordlicht.pc DESTINATION lib/pkgconfig)
