project(nordlicht C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(FreeImage REQUIRED)
include_directories(${FreeImage_INCLUDE_DIRS})

include_directories(include)

add_library(nordlicht SHARED src/common.c src/graphics.c src/nordlicht.c src/video.c)
target_link_libraries(nordlicht ${FFMPEG_LIBRARIES} ${FreeImage_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(nordlicht-tool src/nordlicht-tool.c)
set_target_properties(nordlicht-tool PROPERTIES OUTPUT_NAME nordlicht)
target_link_libraries(nordlicht-tool nordlicht)

set(LIB_SUFFIX "" CACHE STRING "Suffix of library directory name (for example '32' or '64')")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where library will be installed")

configure_file(res/nordlicht.pc.in res/nordlicht.pc @ONLY)

install(TARGETS nordlicht DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS nordlicht-tool DESTINATION bin)
install(FILES src/nordlicht.h DESTINATION include)
install(FILES ${CMAKE_BINARY_DIR}/res/nordlicht.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
