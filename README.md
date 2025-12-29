# Platform Windows SDFX Engine 2D & 3D Raylib
![Badge en Desarollo](https://img.shields.io/badge/STATUS-EN%20DESAROLLO-green)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-green.svg)
![CMake](https://img.shields.io/badge/CMake-Required-blue?logo=cmake)
![NMake](https://img.shields.io/badge/NMake-Supported-lightgrey)

An Open Source 2D & 3D Game Engine

## 👨‍💻 Developers group

SDFX Grupe

## ⚖️ LICENSE

Platform
Copyright (C) 2025 agustinsdfx

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3.

## 📖 ABOUT

Platform is a 2D game engine that allows you to customize the source code and port it to different platforms using CMake and NMAKE, with the Raylib graphics library and the C programming language.

By default, the game comes ported to the latest official version for Windows x64 devices from the GitHub repository: https://github.com/agustinsdfx/Platform-Windows-SDFX-Engine-Raylib

## ⚙ Supported Systems

- Windows x64 platforms

Windows  7, 8, 8.1, 10, 11 and later.

## 👥 Credits

- -Toby Fox (the original creator of the Undertale music; I don't have the licenses, but the songs are optional and you can replace them with others)
- -Creators of Raylib (creators of the graphics library, sound, etc.)
- -Creator of Inno Setup
- -Messiah sound creators (The original "oof" death sound in my game was taken from the game Messiah. I don't own the rights to it, but having that sound is optional; you can replace it with one of your choice.)

My engine/game is only licensed for the code and its own sprites that come by default when you download the code or the compiled build.

## 🛠️ Everything that was used for the development of the game and the engine

### Tools used

- Git

- CMake

- Libresprite

- Visual Studio Code

- Python 3.12.7

### Libraries used

- SDL3

- Raylib

### Languages ​​used

- C & C++

- Python

## 📦 Instructions

- How to get to the libraries

```
git clone --branch libs https://github.com/agustinsdfx/Platform.git
```

Or you can also download the libs branch and add those libraries to your code.

- How to build and compile it

Create the folder for the compilation

For OpenGL 3.3

```
cmake -S . -B ./build
```

For OpenGL 4.2

```
cmake -S . -B ./build -DGRAPHICS=GRAPHICS_API_OPENGL_42
```

- Compile in debug mode

```
cmake --build ./build --config Debug
```

- Compile in release mode

```
cmake --build ./build --config Release
```

## 🗿 Developer Notes

All versions are saved in the folder C:/Games/Platform/version-(vID)

The initial development version is a portable file, although it has the option to take a screenshot with F12; it's just saved along with the folder where the platform file is located.

## 👓 Recommendations

Nothing yet :(

## 💻 System Requirements

Minimum:

- OS: Linux (64-bit)

- Processor: Intel Core i3-12100 / AMD Ryzen 3 4100 or equivalent

- Memory: 4 GB RAM

- Graphics: Intel UHD Graphics 730 / AMD Radeon Graphics (Integrated)

- Storage: 100 MB available space

- Additional Notes: The game process utilizes approximately 700 MB of RAM.

Recommended:

- OS: Linux (64-bit)

- Processor: Intel Core i3-13100 / AMD Ryzen 5 4500

- Memory: 8 GB RAM

- Graphics: Intel UHD Graphics 750 or discrete GPU

- Storage: 100 MB available space (SSD recommended)

## 📜 Changelog

You can see the changes and versions in the file [CHANGELOG.md](CHANGELOG.md).

## 🎨 Community and mods

Visit Platform DB (PL.DB) to download mods, games, textures, and more, as well as view the community forum.

Check it out: https://agustinsdfx.github.io/PlatformDB/

# End of README.md

Thank you for reading this and learning about my platform game and its features.
