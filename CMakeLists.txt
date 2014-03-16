cmake_minimum_required(VERSION 2.8)

project(nordlicht C)
set(NORDLICHT_VERSION 0.3.4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(LIB_SUFFIX "" CACHE STRING "Suffix of library directory name (for example '32' or '64')")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where library will be installed")

find_package(Threads REQUIRED)
find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
find_package(PNG REQUIRED)
find_package(Popt REQUIRED)

include_directories(${CMAKE_BINARY_DIR}/src ${CMAKE_SOURCE_DIR}/src ${FFMPEG_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${POPT_INCLUDES})

add_library(nordlicht SHARED src/common.c src/graphics.c src/nordlicht.c src/video.c)
set_target_properties(nordlicht PROPERTIES SOVERSION 0)
target_link_libraries(nordlicht ${FFMPEG_LIBRARIES} ${PNG_LIBRARIES} ${POPT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(nordlicht-bin src/main.c)
set_target_properties(nordlicht-bin PROPERTIES OUTPUT_NAME nordlicht)
target_link_libraries(nordlicht-bin nordlicht)

add_executable(testsuite tests/library.c)
target_link_libraries(testsuite nordlicht)
add_custom_target(test COMMAND testsuite COMMAND ${CMAKE_SOURCE_DIR}/tests/tool.sh)

configure_file(cmake/nordlicht.pc.in cmake/nordlicht.pc @ONLY)
configure_file(src/version.h.in src/version.h @ONLY)
add_custom_command(TARGET nordlicht-bin POST_BUILD
    COMMAND help2man ${CMAKE_BINARY_DIR}/nordlicht -N -n "creates colorful barcodes from video files"
    -o ${CMAKE_BINARY_DIR}/nordlicht.1
)

install(TARGETS nordlicht DESTINATION ${LIB_INSTALL_DIR})
install(TARGETS nordlicht-bin DESTINATION bin)
install(FILES src/nordlicht.h DESTINATION include)
install(FILES ${CMAKE_BINARY_DIR}/cmake/nordlicht.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/nordlicht.1 DESTINATION share/man/man1)
