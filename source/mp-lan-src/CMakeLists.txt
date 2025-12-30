cmake_minimum_required(VERSION 3.10)

project(Platform)

file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_subdirectory("raylib")

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC raylib ws2_32 winmm)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC raylib)
endif()