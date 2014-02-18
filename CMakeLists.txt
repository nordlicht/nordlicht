project(nordlicht C)
cmake_minimum_required(VERSION 2.8)

set(NORDLICHT_VERSION 0.3.0)
configure_file(src/version.h.in src/version.h @ONLY)
include_directories(${CMAKE_BINARY_DIR}/src)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(Threads REQUIRED)

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIRS})

find_package(FreeImage REQUIRED)
include_directories(${FreeImage_INCLUDE_DIRS})

find_package(Popt REQUIRED)
include_directories(${POPT_INCLUDES})

include_directories(include)

add_library(libnordlicht SHARED src/common.c src/graphics.c src/nordlicht.c src/video.c)
target_link_libraries(libnordlicht ${FFMPEG_LIBRARIES} ${FreeImage_LIBRARIES} ${POPT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(nordlicht src/tool.c)
set_target_properties(nordlicht PROPERTIES OUTPUT_NAME nordlicht)
target_link_libraries(nordlicht libnordlicht)

enable_testing()
add_test(test-tool ${CMAKE_SOURCE_DIR}/test/test-tool.sh)

set(LIB_SUFFIX "" CACHE STRING "Suffix of library directory name (for example '32' or '64')")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where library will be installed")

configure_file(res/nordlicht.pc.in res/nordlicht.pc @ONLY)

install(TARGETS libnordlicht DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS nordlicht DESTINATION bin)
install(FILES src/nordlicht.h DESTINATION include)
install(FILES ${CMAKE_BINARY_DIR}/res/nordlicht.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
