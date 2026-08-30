cmake_minimum_required(VERSION 3.13...3.25)
project(
    LearnWebGPU # name of the project, which will also be the name of the visual studio solution if you use it
    VERSION 0.1.0
    LANGUAGES CXX C
)

# This is so that VScode can use the configurations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(App 
    main.cpp 
    source_utils/webgpu_utils.cpp
    header_utils/webgpu_utils.hpp
)
# don't add source_utils/webgpu_utils.cpp as a subdirectory, then it's build't seperatly to Dawn and main, therefore we get problems with linking


if(NOT EMSCRIPTEN)
    # Add the 'glfw' directory, which contains the definition of a 'glfw' target
    add_subdirectory(glfw)
else()
    # Create a mock 'glfw' target that just sets the `-sUSE_GLFW=3` link option:
    add_library(glfw INTERFACE)
    target_link_options(glfw INTERFACE -sUSE_GLFW=3) # EMSCRIPTEN has built in support
endif()

# When using Dawn, make sure to add the glfw directory before you add webgpu, otherwise Dawn provides its own version (which may be fine sometimes, but you don’t get to chose the version).
add_subdirectory(Dawn) # adding WebGPU Dawn library
add_subdirectory(glfw3webgpu)


# This time, the name ‘webgpu’ is one of the target defined in webgpu/CMakeLists.txt by calling add_library(webgpu ...), 
# it is not related to a directory name.
#target_link_libraries(App PRIVATE WebGPU-distribution-dawn-6536)
target_link_libraries(App PRIVATE webgpu glfw glfw3webgpu)
target_copy_webgpu_binaries(App) # only exists if Dawn defines it This line

# this does not seam to work at all.... might just be that webgpu is marked as a library and thats why we can find webgpu/webgpu.hpp
target_include_directories(App PRIVATE  
    ${CMAKE_CURRENT_SOURCE_DIR}/header_utils
)
#    ${CMAKE_CURRENT_SOURCE_DIR}/webgpu  # Wherever webgpu.hpp lives

# We add an option to enable different settings when developing the app than
# when distributing it.
option(DEV_MODE "Set up development helper settings" ON)


if(DEV_MODE)
    # In dev mode, we load resources from the source tree, so that when we
    # dynamically edit resources (like shaders), these are correctly
    # versionned.
    target_compile_definitions(App PRIVATE
        RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources"
    )
else()
    # In release mode, we just load resources relatively to wherever the
    # executable is launched from, so that the binary is portable
    target_compile_definitions( App PRIVATE
       RESOURCE_DIR="./resources"
    )
endif()
# You can then have 2 different builds of your project in two different directories:
# cmake -B build-dev -DDEV_MODE=ON -DCMAKE_BUILD_TYPE=Debug
# cmake -B build-release -DDEV_MODE=OFF -DCMAKE_BUILD_TYPE=Release
# CMAKE_BUILD_TYPE, sets in this context eather debug flags or mot optimization flags in Release
# CMAKE_BUILD_TYPE is ignored in some CMake Generators, like the Visual Studio one where solutions can switch in the IDE instead 

# effects compiler flas for the compiler and more...?
set_target_properties(App PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON 
        #COMPILE_WARNING_AS_ERROR ON #We don't need it?
)

# To make sure we even have as many warnings as possible, we add some compile options:
if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

# build\Debug\App.exe  # Windows
#///////////////////////////////////////////////////////////////////////?
#In the accompanying code, I hide these details in the target_treat_all_warnings_as_errors() function defined in 
#utils.cmake and included at the beginning of the CMakeLists.txt.
#///////////////////////////////////////////////////////////////////////?

if (XCODE)
    set_target_properties(App PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()


# Because we are using Dawn(writen in c++) we don't need to dynamicly link, There are no precompiled binaries
# The application's binary must find wgpu.dll or libwgpu.so at runtime,
# so we automatically copy it (it's called WGPU_RUNTIME_LIB in general)
# next to the binary.
#target_copy_webgpu_binaries(App)


# building for the web
if(EMSCRIPTEN)
    # Generate a full web page rather than a simple WebAssembly module
    set_target_properties(App PROPERTIES SUFFIX ".html")

    # Enable the use of emscripten_sleep()
    target_link_options(App PRIVATE 
        -sASYNCIFY
        --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/resources@resources"
        #  preciseing @resources mean that the directory must be mounted at path /resources, By default, the path of the preloaded file relative to CMakeLists.txt is used.
    )
endif()



#Cmake remmember:
#instead of building default target witch leads to dependecy cheching of Dawns/webgpu's dependencys
# it checks so that the dependencyes are up to date, so instead of cmake --build build , use
# cmake --build build --target App , witch only targets the relevant main

