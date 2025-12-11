# Pathological

Pathological will be a distributed C++/Vulkan Path tracer.

This project takes it's name from [Pathological](https://www.github.com/PeterGottesman/Pathological). That was the first C++ project I wrote back in college, and was used a few years later as the base of a project for students Univeristy of Louisville's CSE 496: BACS Capstone.

This iteration of the project - "V2"- is being created solely to be used for the same University of Louisville class. I am initiating a complete rewrite with the goal of having improved code quality and improved performance through hardware acceleration. This should reflect this project's purpose as a teaching tool rather than being my toy intro to c++ project as the original was.

## Installing

### Dependencies 

Dependencies for this project are managed by `vcpkg`, although the following should be installed externally
- Vulkan SDK - See https://vulkan.lunarg.com/doc/sdk/latest/linux/getting_started.html for installation instructions & dependencies
- Graphics Driver Supporting Vulkan Raytracing
- vcpkg
- cmake
- ninja-build
- xinerama
- xcursor
- xorg
- libglu1-mesa
- pkg-config

### Compiling

TODO: Fill out this section more

```
cmake --preset default
cmake --build build
```
