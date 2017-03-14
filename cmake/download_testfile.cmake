if(NOT EXISTS ${CMAKE_BINARY_DIR}/video.mp4)
    message("Downloading 3 MB test file, just a sec...")
    file(DOWNLOAD https://upload.wikimedia.org/wikipedia/commons/6/64/Sintel_excerpt.ogv ${CMAKE_BINARY_DIR}/video.mp4 EXPECTED_MD5 be22cb1f83f6f3620d8048df12f9f52d)
endif()
