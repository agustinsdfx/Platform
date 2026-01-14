#!/bin/bash

BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

clear
echo -e "${BLUE}==========================================${NC}"
echo -e "${BLUE}             PLATFORM BUILDER             ${NC}"
echo -e "${BLUE}==========================================${NC}"

echo -e "Select OpenGL Version:"
echo -e "1) OpenGL 1.1"
echo -e "2) OpenGL 2.1"
echo -e "3) OpenGL 3.3"
echo -e "4) OpenGL 4.3"
echo -e "q) Exit"
read -p "Selection: " opt_gl

case $opt_gl in
    1) GL_VERSION="GRAPHICS_API_OPENGL_11" ;;
    2) GL_VERSION="GRAPHICS_API_OPENGL_21" ;;
    3) GL_VERSION="GRAPHICS_API_OPENGL_33" ;;
    4) GL_VERSION="GRAPHICS_API_OPENGL_43" ;;
    q) exit 0 ;;
    *) echo -e "${RED}Invalid option${NC}"; exit 1 ;;
esac

echo -e "\nSelect Build Mode:"
echo -e "1) Debug"
echo -e "2) Release"
echo -e "q) Exit"
read -p "Selection: " opt_mode

case $opt_mode in
    1) BUILD_TYPE="Debug" ;;
    2) BUILD_TYPE="Release" ;;
    q) exit 0 ;;
    *) echo -e "${RED}Invalid option${NC}"; exit 1 ;;
esac

echo -e "\n${GREEN}>>> Cleaning build cache...${NC}"
if [ -f "./build/CMakeCache.txt" ]; then
    rm ./build/CMakeCache.txt
fi

echo -e "${GREEN}>>> Configuring CMake...${NC}"
echo -e "Target API: ${BLUE}$GL_VERSION${NC}"
echo -e "Target Mode: ${BLUE}$BUILD_TYPE${NC}\n"

cmake -S . -B ./build -DGRAPHICS=$GL_VERSION -DCMAKE_BUILD_TYPE=$BUILD_TYPE

if [ $? -eq 0 ]; then
    echo -e "\n${GREEN}>>> Build configuration ready. Starting compilation...${NC}"
    cmake --build ./build --config $BUILD_TYPE
    
    if [ $? -eq 0 ]; then
        echo -e "\n${GREEN}=========================================${NC}"
        echo -e "${GREEN}              BUILD SUCCESSFUL!            ${NC}"
        echo -e "${GREEN}===========================================${NC}"
    else
        echo -e "\n${RED}Error: Compilation failed.${NC}"
    fi
else
    echo -e "\n${RED}Error: CMake configuration failed.${NC}"
fi