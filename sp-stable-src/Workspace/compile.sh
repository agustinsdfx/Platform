#!/bin/bash

BLUE='\033[0;34m'
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${BLUE}=== Platform Experimental Build System ===${NC}"

echo "Select OpenGL Version:"
echo "1) OpenGL 3.3 (Legacy/Compatible)"
echo "2) OpenGL 4.3 (Modern/Compute Shaders)"
read -p "Option [1-2]: " opt_gl

if [ "$opt_gl" == "2" ]; then
    GL_VERSION="GRAPHICS_API_OPENGL_43"
else
    GL_VERSION="GRAPHICS_API_OPENGL_33"
fi

echo -e "\nSelect Build Mode:"
echo "1) Debug"
echo "2) Release"
read -p "Option [1-2]: " opt_mode

if [ "$opt_mode" == "1" ]; then
    BUILD_TYPE="Debug"
else
    BUILD_TYPE="Release"
fi

echo -e "\n${GREEN}Cleaning and configuring...${NC}"
echo -e "API: ${BLUE}$GL_VERSION${NC}"
echo -e "MODE: ${BLUE}$BUILD_TYPE${NC}"

if [ -f "./build/CMakeCache.txt" ]; then
    rm ./build/CMakeCache.txt
fi

cmake -S . -B ./build -DGRAPHICS=$GL_VERSION -DCMAKE_BUILD_TYPE=$BUILD_TYPE

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Config success. Building...${NC}"
    cmake --build ./build --config $BUILD_TYPE
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}Build complete!${NC}"
    else
        echo "Build failed."
    fi
else
    echo "Configuration failed."
fi