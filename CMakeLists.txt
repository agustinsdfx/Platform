cmake_minimum_required(VERSION 3.10)

project(Platform)

file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)

add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
    target_link_options(${PROJECT_NAME} PRIVATE /ENTRY:mainCRTStartup)
endif()

add_subdirectory("raylib")

target_link_libraries(${PROJECT_NAME} PUBLIC raylib)