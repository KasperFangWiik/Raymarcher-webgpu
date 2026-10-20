# Building and executing
call the commands under 
cmake -S . -B build
cmake --build build
Then the executeble should be called App and be placed in ./build/Debug directory

# inspiration and general information:
The project renders procidual terrain using raymarching tecniques in WebGPU. 
It also tests cone-marching for optimization.
This project fokused on learning the WebGPU standard, Especially fokused on Interfacing with it 
using C++. For now it's nativly built using Googles implementation of the WebGPU standard called Dawn.
So the goal was to learn some WebGPU while creating something intressting and funn.

The webtutorial "WebGPU For native graphics in C++"( https://eliemichel.github.io/LearnWebGPU/ ) 
was substantially used. The article "Cone Marching in Three.js" (https://medium.com/@nabilnymansour/cone-marching-in-three-js-6d54eac17ad4)
was also used for implementing cone-marching for optimization. 
Inigo Quilez video "I painted a Landscape with Mathematics"(https://www.youtube.com/watch?v=BFld4EBO2RE&t=912s)
was used for infereing the logic underlying the rendering of the procidual terrain.
