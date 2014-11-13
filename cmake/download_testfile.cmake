if(NOT EXISTS ${CMAKE_BINARY_DIR}/video.mp4)
    message("Downloading 20 MB test file, just a sec...")
    file(DOWNLOAD https://upload.wikimedia.org/wikipedia/commons/7/79/Big_Buck_Bunny_small.ogv ${CMAKE_BINARY_DIR}/video.mp4 EXPECTED_MD5 9f701e645fd55e1ae8d35b7671002881)
endif()
