cmake_minimum_required(VERSION 2.8)
#project(vidcode)

add_library(vidcode SHARED vidcode.c)
#add_executable(main main.c)
#target_link_libraries(main vidcode)
install(TARGETS vidcode DESTINATION lib)
install(FILES vidcode.h DESTINATION include)
