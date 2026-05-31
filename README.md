# bachelors-demo
This repository contains an implementation of an unassisted project (B4BPROJ6) course at FEE CTU. 


> [!WARNING]
> This software was tested and confirmed working on macOS and Linux. It was not tested and therefore is not guaranteed to compile/run on Windows as at the moment I do not have access to a computer running Windows.

### Runtime dependencies 
- ```glm``` >= 1.0.0
- ```glfw``` >= 3.3
- ```opengl``` >= 3.3
- ```opencl``` >= 1.2
- ```openvdb``` >= 13.0.0

### Build dependencies
- ```cmake``` >= 3.30.0
- C compiler with **C99** support
- C++ compiler with **C++17** support

### Usage
Program is invoked as ```raytracer [model] (flags)```. Model (in .vdb format, must contain grid named "density") is required and must be provided as the first argument. Benchmarking mode can be started by using ```-b/--benchmark``` flag along with the duration of the benchmark.

> [!NOTE]
> Model must contain grid named "density" to be properly parsed. This application was tested using [Walt Disney Animation Studio's dataset](https://www.disneyanimation.com/resources/clouds/).


## Acknowledgements
This project uses and redistributes [```glad```](https://github.com/Dav1dde/glad) OpenGL Loader <br />
Copyright (c) 2013-2022 David Herberth, licensed under [MIT](https://github.com/Dav1dde/glad/blob/glad2/LICENSE) License

This project uses and redistributes parts of the Khronos Specifications (as a part of glad) <br />
Copyright (c) 2013-2020 The Khronos Group Inc, licensed under [Apache License v. 2.0](https://github.com/Dav1dde/glad/blob/glad2/LICENSE)
