find_package(AWSSDK REQUIRED COMPONENTS s3)

add_library(s3manager STATIC ${CMAKE_CURRENT_SOURCE_DIR}/s3_manager.cpp)
target_link_libraries(s3manager PUBLIC ${AWSSDK_LINK_LIBRARIES})
target_include_directories(s3manager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
