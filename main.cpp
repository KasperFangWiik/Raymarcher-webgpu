//#include <webgpu/webgpu.hpp>
#include<GLFW/glfw3.h>
#include<iostream>
#include<vector>

#include"header_utils/webgpu_utils.hpp"
#include<webgpu/webgpu.hpp>

#include<chrono>

// If using Dawn, can i define WEBGPU_BACKEND_DAWN while building..
#ifndef WEBGPU_BACKEND_DAWN
	#define WEBGPU_BACKEND_DAWN
#endif

/*
// If using wgpu-native
#ifndef WEBGPU_BACKEND_WGPU
	#define WEBGPU_BACKEND_WGPU
#endif

// If using emscripten
#ifndef WEBGPU_BACKEND_EMSCRIPTEN
	#define WEBGPU_BACKEND_EMSCRIPTEN
#endif
*/

#ifdef __EMSCRIPTEN__
	#include<emscripten.h>
#endif

#ifdef WEBGPU_BACKEND_WGPU
	#include <webgpu/wgpu.h>
#endif // WEBGPU_BACKEND_WGPU


int HelloApplication(){
	Application app;

	if(!app.Initialize()){
		return 1;
	}

	#ifdef __EMSCRIPTEN__
		// Emscripten main loop, aka setup of mainloop callback
		auto callback = [](void* arg){
			Application* pApp = reinterpret_cast<Applictaion*>(arg);
			pApp->MainLoop();
		};
		emscripten_set_main_loop_arg(callback, &app, 0, true);
	#else // __EMSCRIPTEN__
		while(app.IsRunning()){
			app.MainLoop();
		}
	#endif // __EMSCRIPTEN__

	app.Terminate();
	return 0;
}

int main(int, char**) {

	std::cout << "start main..." << std::endl;
	Application app;

	if(!app.Initialize()){
		return 1;
	}

	#ifdef __EMSCRIPTEN__
		// Emscripten main loop, aka setup of mainloop callback
		auto callback = [](void* arg){
			Application* pApp = reinterpret_cast<Applictaion*>(arg);
			pApp->MainLoop();
		};
		emscripten_set_main_loop_arg(callback, &app, 0, true);
	#else // __EMSCRIPTEN__
		while(app.IsRunning()){
			app.MainLoop();
		}
	#endif // __EMSCRIPTEN__

	app.Terminate();

	return 0;
}

/*
GIT repo!!!!
Move Raymarcher to own git repo.
Find ways to giv inline error messages for WGSL.

1. movement(mouse input), maby wasd
2. stop time 
3. FPS
4. resizing of window
5. få den att compilera för emscripten och fö wgpu-native, Sedan hur får du den på nätet? Visas på Git-hub?

6. Cone-marching
*/