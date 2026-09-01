#pragma once
#include<webgpu/webgpu.hpp>
#include<GLFW/glfw3.h> // should we inlcude this here
#include<glfw3webgpu.h>
#include<chrono>
#include<utility>
#include<vector>
#include<array>
// witch are needed......
#include<filesystem>
#include<fstream>
#include<sstream>
#include<string>


// If using Dawn
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

// probably should have it here to include emscripten_sleep(100);
#ifdef __EMSCRIPTEN__
	#include<emscripten.h>
#endif

////////////////////////////////////////////////////////////////////////////////////////
///// Instans and Descriptor
////////////////////////////////////////////////////////////////////////////////////////
bool getInstance(WGPUInstanceDescriptor& desc, WGPUInstance& instance);
// we will start with having simple structs that include the "objects" that share lifetimes
// Descriptor desc and TogglesDescriptor toggles does not need to exists for the life time of WGPUInstance instance!!!!!!!

////////////////////////////////////////////////////////////////////////////////////////
///// Adapter
////////////////////////////////////////////////////////////////////////////////////////
WGPUAdapter requestAdapterSync(
	WGPUInstance instance, 
	WGPURequestAdapterOptions const * options
);

WGPUAdapter getAdapter(
	WGPUInstance instance,
	WGPURequestAdapterOptions options
);

void getAdapterLimits(WGPUAdapter adapter);
void getAdapterFeatures(WGPUAdapter adapter);
void getAdapterProperties(WGPUAdapter adapter);

void setDefaultLimits(WGPULimits &limits); // should i use references for WGPU

////////////////////////////////////////////////////////////////////////////////////////
///// Device
////////////////////////////////////////////////////////////////////////////////////////
WGPUDevice requestDeviceSync(
	WGPUAdapter adapter, 
	WGPUDeviceDescriptor const* descriptor
);

void inspectDevice(WGPUDevice device);
void setDeviceDescriptor(
	WGPUDeviceDescriptor& deviceDesc, 
	WGPURequiredLimits& limits,
	size_t featureCount,
	WGPUFeatureName const* featutes
); // sets Default values for descriptor
WGPUDevice getDefaultDevice(WGPUAdapter adapter);
WGPUDevice getNewDeviceAutoCallback(WGPUDeviceDescriptor& deviceDesc, WGPUAdapter adapter);

////////////////////////////////////////////////////////////////////////////////////////
//// Queue
////////////////////////////////////////////////////////////////////////////////////////
WGPUQueue getQueue(WGPUDevice device);


////////////////////////////////////////////////////////////////////////////////////////
//// Configure Surface
////////////////////////////////////////////////////////////////////////////////////////
void configureSurfaceSetup( // should this be application class function?, but we should also have function for printin the surface's capabilities...
	WGPUDevice device,
	WGPUAdapter adapter, 
	WGPUSurface surface,
	WGPUTextureFormat& surfaceFormat, // should be a pointer to formats/array of formats...
	uint32_t window_width,
	uint32_t window_height
 );



////////////////////////////////////////////////////////////////////////////////////////
//// Asynchronous device polling, wgpuPollEvents
////////////////////////////////////////////////////////////////////////////////////////
// We define a function that hides implementation-specific variants of device polling:
//void wgpuPollEvents( [[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser);


////////////////////////////////////////////////////////////////////////////////////////
//// Renderpass
////////////////////////////////////////////////////////////////////////////////////////
WGPURenderPassColorAttachment getColorAttachment(WGPUTextureView targetView);
WGPURenderPassEncoder encodeRenderPass(
	WGPUCommandEncoder encoder, 
	WGPUTextureView targetView, 
	WGPURenderPassTimestampWrites* timestampWrites
);

/*
Uniform info:...
*/
struct UniformStruct {
	std::array<float, 2> iResolution;
	std::array<float, 2> iMouse;
	float iTime;
	float _pad;
};
// Have the compiler check byte alignment
static_assert(sizeof(UniformStruct) % 8 == 0);


void setDefault(WGPUBindGroupLayoutEntry &bindingLayout);

WGPURenderPipeline getVertexMultAttrRenderPipeline(
	WGPUDevice device,
	WGPUTextureFormat& surfaceFormat,
	WGPUPipelineLayout& layout,
    WGPUBindGroupLayout& bindGroupLayout,
	WGPUBindGroup& bindGroup,
	WGPUBuffer& uniformBuffer,
	WGPUTextureView& textureView,
	WGPUSampler& sampler
);

////////////////////////////////////////////////////////////////////////////////////////
//// Load ShaderModule from file in resources
////////////////////////////////////////////////////////////////////////////////////////
WGPUShaderModule loadShaderModule(const std::filesystem::path& path, WGPUDevice device);

////////////////////////////////////////////////////////////////////////////////////////
//// Application
////////////////////////////////////////////////////////////////////////////////////////
class Application {
	public:
		bool Initialize();
		void MainLoop();
		bool IsRunning(); // true as long as the main loop should be running
		void Terminate();

	private:
		uint32_t window_width;
		uint32_t window_height;
		// numbOfVertexAttributes
		// subdivisions
		WGPUTexture grayNoise;
		WGPUTextureView grayNoiseView;
		WGPUSampler sampler;

		WGPUInstance instance;
		WGPUAdapter adapter;
		WGPUDevice device;
		WGPUQueue queue;

		WGPUSurface surface;
		GLFWwindow* window;

		WGPUTextureFormat surfaceFormat = WGPUTextureFormat_Undefined;
		WGPURenderPipeline pipeline;

		std::pair<WGPUSurfaceTexture, WGPUTextureView> getNextSurfaceViewData();
		WGPURequiredLimits setRequredLimits(/*WGPUAdapter adapter*/) const; // what dose it mean when const comes after the function declaration?

		WGPUBuffer vertexBuffer;
		uint32_t vertexCount;
		void InitializeIndexBuffers();
		void InitializeConemarchIndexBuffers();
		WGPUBuffer pointBuffer;
		WGPUBuffer indexBuffer;
		uint32_t indexCount;

		WGPUPipelineLayout layout;
    	WGPUBindGroupLayout bindGroupLayout; // Retaining bindGroupLayout is also needed when we create the bind group that goes with it.
		WGPUBindGroup bindGroup; // A bind group must follow the same structure as its layout
		WGPUBuffer uniformBuffer;
		void InitializeUniformStructBuffer();

		WGPUQuerySet timestampQuerySet;
		void InitializeTimestampBuffer();
		WGPUBuffer timestampBuffer;
		WGPUBuffer mappedTimeBuffer;
		uint64_t numberOfQuerySets; // should be const... 
		uint64_t querySetCounter;

		bool ready;// = false; // does this work for initilization....

		WGPUTextureView InitializeGrayNoiseTexture();
		void InitializeTextureSampler();
		WGPUTextureView CreatEmptyGrayNoiseTexture();	
		void ComputePassCreatTexture();
		WGPUBindGroupLayout computeBindGroupLayout; // Retaining bindGroupLayout is also needed when we create the bind group that goes with it.
		WGPUBindGroup computeBindGroup;
		WGPUPipelineLayout computeLayout;
		WGPUComputePipeline computePipeline;
	};