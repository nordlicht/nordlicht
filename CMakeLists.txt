cmake_minimum_required(VERSION 2.8)

project(nordlicht C)
set(MAJOR_VERSION 0)
set(MINOR_VERSION 4)
set(PATCH_VERSION 5)
set(NORDLICHT_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")

option (BUILD_SHARED_LIBS "Build shared libraries." ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(GNUInstallDirs)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

if (BUILD_SHARED_LIBS)
    add_definitions(-DNORDLICHT_BUILD_SHARED)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
else()
    add_definitions(-DNORDLICHT_BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

find_package(FFmpeg COMPONENTS AVUTIL AVFORMAT AVCODEC SWSCALE REQUIRED)
find_package(Popt REQUIRED)

if (NOT WIN32)
    find_program(HELP2MAN help2man)
    if (NOT HELP2MAN)
        message(FATAL_ERROR "help2man not found, aborting.")
    endif()
endif()

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${FFMPEG_INCLUDE_DIRS} ${POPT_INCLUDES})

add_library(libnordlicht src/error.c src/image.c src/nordlicht.c src/source.c)
set_target_properties(libnordlicht PROPERTIES OUTPUT_NAME nordlicht)
set_target_properties(libnordlicht PROPERTIES SOVERSION ${MAJOR_VERSION} VERSION ${NORDLICHT_VERSION})
set_target_properties(libnordlicht PROPERTIES C_VISIBILITY_PRESET hidden)
target_link_libraries(libnordlicht m ${FFMPEG_LIBRARIES})

add_executable(nordlicht src/main.c)
target_link_libraries(nordlicht libnordlicht ${POPT_LIBRARIES})
if (WIN32)
    target_link_libraries(nordlicht mman)
endif()

add_executable(testsuite src/testsuite.c)
target_link_libraries(testsuite libnordlicht)

configure_file(src/version.h.in version.h @ONLY)

if (NOT WIN32)
    configure_file(cmake/nordlicht.pc.in nordlicht.pc @ONLY)
    add_custom_command(TARGET nordlicht POST_BUILD
        COMMAND help2man ${CMAKE_BINARY_DIR}/nordlicht -N -n "creates colorful timebars from video and audio files"
        -o ${CMAKE_BINARY_DIR}/nordlicht.1
    )
endif()

install(TARGETS libnordlicht DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS nordlicht DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/src/nordlicht.h DESTINATION include)
install(FILES ${CMAKE_BINARY_DIR}/nordlicht.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
install(FILES ${CMAKE_BINARY_DIR}/nordlicht.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install(FILES ${CMAKE_SOURCE_DIR}/utils/mpv-nordlicht.lua DESTINATION share/nordlicht)
install(FILES ${CMAKE_SOURCE_DIR}/utils/nordlicht.thumbnailer DESTINATION share/nordlicht)

add_custom_target(check testsuite)
add_custom_target(download_testfile ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/download_testfile.cmake)
add_dependencies(check libnordlicht nordlicht download_testfile)
