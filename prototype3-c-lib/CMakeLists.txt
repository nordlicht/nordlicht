project(vidcode C)
cmake_minimum_required(VERSION 2.8)

add_library(vidcode SHARED vidcode.c)

install(TARGETS vidcode DESTINATION lib)
install(FILES vidcode.h DESTINATION include)
