add_library(common mytcp.c mytcp.h common.c common.h)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

