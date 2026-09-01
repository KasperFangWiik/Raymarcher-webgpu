#include"webgpu_utils.hpp"
////////////////////////////////////////////////////////////////////////////////////////
///// Instans and Descriptor
////////////////////////////////////////////////////////////////////////////////////////
// WGPUInstanceDescriptor& desc, when should i use move semantics?
// we should probably not have WGPUInstance& instance in parameter list...
bool getInstance(WGPUInstanceDescriptor& desc, WGPUInstance& instance){ // is there no problem with life times...
	std::cout << "Creating instance..." << std::endl;
	// The descriptor is a kind of way to pack many function arguments together, because some descriptors really have a lot of fields.
	//desc.nextInChain = nullptr;

	#ifdef WEBGPU_BACKEND_DAWN
		WGPUDawnTogglesDescriptor toggles = {};
		// Makse sure the uncaptured error callback is called as soon as an error
		// occurs rather than at the next call to "wgpuDeviceTick".
		//WGPUDawnTogglesDescriptor toggles;
		toggles.chain.next = nullptr;
		toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
		toggles.disabledToggleCount = 0;
		toggles.enabledToggleCount = 1;
		 // will probably go outside of scoop(not live the hoal main...)
		const char* toggleName = "enable_immediate_error_handling";
		toggles.enabledToggles = &toggleName;
		desc.nextInChain = &toggles.chain;
	#endif // WEBGPU_BACKEND_DAWN

	#ifdef WEBGPU_BACKEND_EMSCRIPTEN
		instance = wgpuCreateInstance(nullptr);
	#else //  WEBGPU_BACKEND_EMSCRIPTEN
		instance = wgpuCreateInstance(&desc);
	#endif //  WEBGPU_BACKEND_EMSCRIPTEN

	if (!instance){
		std::cerr << "Could not initilize WebGPU" << std::endl;
		return false;
	}

	std::cout << "Instance created! " << std::endl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
///// Adapter
////////////////////////////////////////////////////////////////////////////////////////
/// THIS funcion was taken from learn WebGPU c++ should add the commment 
WGPUAdapter requestAdapterSync(
	WGPUInstance instance, 
	WGPURequestAdapterOptions const * options
){
	// Simple structure holding local information shared with
	// onAdapterRequestEnded callback.
	struct UserData{
		WGPUAdapter adapter = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onAdapterRequestedEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData){
		//UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		UserData& userData = *static_cast<UserData*>(pUserData);
		if(status == WGPURequestAdapterStatus_Success){
			userData.adapter = adapter;
		} else {
			std::cout << "Could not get WebGPU adapter: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	// Call to the WebGPU request adapter procedure
	wgpuInstanceRequestAdapter(
		instance, /* equivalent of navigator.gpu */
		options,
		onAdapterRequestedEnded,
		static_cast<void*>(&userData) // (void*)&userData // &userData
	);

	#ifdef __EMSCRIPTEN__
		while(!userData.requestEnded) { // We wait until userData.requestEnded gets true
			emscripten_sleep(100); // Wait
		}
	#endif // __EMSCRIPTEN__

	assert(userData.requestEnded);

	return userData.adapter;
}


// should see if i can qury multiple "divices" and chose
WGPUAdapter getAdapter(
	WGPUInstance instance,
	WGPURequestAdapterOptions options
){
	std::cout << "Requesting adapter..." << std::endl;
	WGPUAdapter adapter = requestAdapterSync(instance, &options);
	std::cout << "Got adapter: " << adapter << std::endl;
	return adapter;
}

/* Limits:
		all maximum and minimum values that may limit behavior of underlying GPU and its driver
		retrieved using: wgpuAdapterGetLimits
*/
void getAdapterLimits(WGPUAdapter adapter){
	// as of April 1st, 2024, wgpuAdapterGetLimits is not implemented yet on Google Chrome
	#ifndef __EMSCRIPTEN__
		WGPUSupportedLimits supportedLimits = {};
		supportedLimits.nextInChain = nullptr;

		// The procedure wgpuAdapterGetLimits returns a boolean in wgpu-native but a WGPUStatus in Dawn.
		#ifdef WEBGPU_BACKEND_DAWN
			bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
		#else
			bool success = wgpuAdapterGetLimits(adapter, &supportedLimits)
		#endif // WEBGPU_BACKEND_DAWN

		if (success) {
			std::cout << "Adapter limits:" << std::endl;
    		std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
    		std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
    		std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
    		std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
			std::cout << " - maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;
		}

	#endif // NOT __EMSCRIPTEN__
}

/* Features:
		non-mandatory extensions of WebGPU, that adapters may or may not support. 
		listed using wgpuAdapterEnumerateFeatures or 
		tested individually with wgpuAdapterHasFeature.
*/
void getAdapterFeatures(WGPUAdapter adapter){
	//std::vector<WGPUFeatureName> features;
	// Call the function a first time with a null return address, just to get the entry count.
	const size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);

	//features.resize(featureCount);
	std::vector<WGPUFeatureName> features(featureCount);

	// Call the function a second time, with non-null return address
	wgpuAdapterEnumerateFeatures(adapter, features.data());

	std::cout << "Adapter features" << std::endl;
	std::cout << std::hex; // // Write integers as hexadecimal to ease comparison with webgpu.h literals
	for (auto const& f : features) {
		std::cout << " - 0x" << f << std::endl;
	}
	std::cout << std::dec; // Restore decimal numb
}

/* Properies:
		extra indo about the adapter, like name, vendor etc.
		using wgpuAdapterGetProperties.
*/
void getAdapterProperties(WGPUAdapter adapter){

	WGPUAdapterProperties properties = {};
	properties.nextInChain = nullptr;
	wgpuAdapterGetProperties(adapter, &properties);

	std::cout << "Adapter properties" << std::endl;
	std::cout << " - vendoeID" << properties.vendorID << std::endl;

	if(properties.vendorName){
		std::cout << " - vendorName: " << properties.vendorName << std::endl;
	}

	if(properties.architecture){
		std::cout << " - architecture: " << properties.architecture << std::endl;
	}

	std::cout << " - deviceID: " << properties.deviceID << std::endl;

	if(properties.name){
		std::cout << " - name: " << properties.name << std::endl;
	}

	if(properties.driverDescription){
		std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
	}

	std::cout << std::hex;
	std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
	std::cout << " - backendType 0x" << properties.backendType << std::endl;
	std::cout << std::dec;
}


void setDefaultLimits(WGPULimits &limits){
	limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureArrayLayers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroups = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindGroupsPlusVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBindingsPerBindGroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicUniformBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxDynamicStorageBuffersPerPipelineLayout = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSampledTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxSamplersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxStorageTexturesPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBuffersPerShaderStage = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxUniformBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxStorageBufferBindingSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.minUniformBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.minStorageBufferOffsetAlignment = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBuffers = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxBufferSize = WGPU_LIMIT_U64_UNDEFINED;
    limits.maxVertexAttributes = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxVertexBufferArrayStride = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderComponents = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxInterStageShaderVariables = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachments = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxColorAttachmentBytesPerSample = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupStorageSize = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeInvocationsPerWorkgroup = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeX = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeY = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupSizeZ = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxComputeWorkgroupsPerDimension = WGPU_LIMIT_U32_UNDEFINED;
}


////////////////////////////////////////////////////////////////////////////////////////
///// Device
////////////////////////////////////////////////////////////////////////////////////////

/* 
A WebGPU device represents a context of use of the API. All the objects that we create 
(geometry, textures, etc.) are owned by the device.

The device is requested from an adapter by specifying the subset of limits and features 
that we are interesed in. Once the device is created, the adapter should no longer be used.
The only capabilities that matter to the application are the ones of the device.
*/

/* Utility function to get a WebGPU device, so that
 *     WGPUDevice device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */

WGPUDevice requestDeviceSync(
	WGPUAdapter adapter, 
	WGPUDeviceDescriptor const* descriptor
){
	struct UserData{
		WGPUDevice device = nullptr;
		bool requestEnded = false;
	};
	UserData userData;

	auto onDeviceRequestEnded = [](
		WGPURequestDeviceStatus status,
		WGPUDevice device,
		char const* message, 
		void* pUserData
	){
		UserData& userData = *reinterpret_cast<UserData*>(pUserData);
		if (status == WGPURequestDeviceStatus_Success) {
			userData.device = device;
		} else {
			std::cout << "Could not get WebGPU device: " << message << std::endl;
		}
		userData.requestEnded = true;
	};

	wgpuAdapterRequestDevice(
		adapter,
		descriptor,
		onDeviceRequestEnded,
		static_cast<void*>(&userData) // (void*)&userData // &userData
	);

	#ifdef __EMSCRIPTEN__ 
		while(!userData.requestEnded) {
			emscripten_sleep(100);
		}
	#endif // __EMSCRIPTEN__
		
	assert(userData.requestEnded);
	
	return userData.device;
}

void inspectDevice(WGPUDevice device){
    //std::vector<WGPUFeatureName> features;
    const size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
    //features.resize(featureCount);
	std::vector<WGPUFeatureName> features(featureCount);
    wgpuDeviceEnumerateFeatures(device, features.data());

    std::cout << "Device features:" << std::endl;
    std::cout << std::hex;
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    WGPUSupportedLimits d_limits = {};
    d_limits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuDeviceGetLimits(device, &d_limits) == WGPUStatus_Success;
#else
    bool success = wgpuDeviceGetLimits(device, &d_limits);
#endif

    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << d_limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << d_limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << d_limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << d_limits.limits.maxTextureArrayLayers << std::endl;
		std::cout << " - maxVertexAttributes: " << d_limits.limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBuffers: " << d_limits.limits.maxVertexBuffers << std::endl;
        // add more if needed
    }
}

void setDeviceDescriptor(
	WGPUDeviceDescriptor& deviceDesc, 
	WGPURequiredLimits& limits,
	size_t featureCount,
	WGPUFeatureName const* featutes
){
	// initialize to a very minimal option,
	// requiring no special feature and using default limits:
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device"; // anything works here

	deviceDesc.requiredFeatureCount = featureCount; // we do not require any specific feature
	//WGPUFeatureName featutes[1] = {WGPUFeatureName_TimestampQuery};
	deviceDesc.requiredFeatures = featutes;

	deviceDesc.requiredLimits = &limits; // we require specific limits
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";

	deviceDesc.deviceLostCallbackInfo.nextInChain = nullptr;
	// deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous; // this is ok if we only logg the error that crached the divce, but if we want to
	// , but if we want to do anyting like recuver from the chrach we need to syncronice the callback function call with wgpuDeviceTick() otherwise we can get race conditions and deadlock problems
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents; // syncronice the callback function call with wgpuDeviceTick()....


}

WGPUDevice getDefaultDevice( WGPUAdapter adapter){	

	WGPUDeviceDescriptor deviceDesc = {};
	// initialize to a very minimal option,
	// requiring no special feature and using default limits:
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device"; // anything works here
	deviceDesc.requiredFeatureCount = 0; // we do not require any specific feature
	deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";

	deviceDesc.deviceLostCallbackInfo.nextInChain = nullptr;
	// deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous; // this is ok if we only logg the error that crached the divce, but if we want to
	// , but if we want to do anyting like recuver from the chrach we need to syncronice the callback function call with wgpuDeviceTick() otherwise we can get race conditions and deadlock problems
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents; // syncronice the callback function call with wgpuDeviceTick()....
	
	//deviceDesc.deviceLostCallbackInfo.userdata; // i assume this is set by the callback function...

	// typedef void (*WGPUDeviceLostCallbackNew)(WGPUDevice const * device, WGPUDeviceLostReason reason, char const * message, void * userdata) WGPU_FUNCTION_ATTRIBUTE;
	deviceDesc.deviceLostCallbackInfo.callback = [](
		WGPUDevice const * device,
		WGPUDeviceLostReason reason, 
		char const* message,
		void* /* pUserData */
	){
		std::cout << "Device lost: reason " << reason;
		if (message) { std::cout << "(" << message << ")";}
		std::cout << std::endl;

	};

	std::cout << "Request device..." << std::endl;

	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);

	// we don't really need adapter after we got device(good practice to delete) // but better to destry selfs later with the outher...
	//wgpuAdapterRelease(adapter); 

	std::cout << "Got device: " << std::endl;


	/* Uncaptured Error Callback
		The uncaptured error callback is invoked whenever we misuse the API, and gives very informative feedback.
		It only set after the creation of the device, by calling wgpuDeviceSetUncapturedErrorCallback:
	*/
	auto onDeviceError = []( WGPUErrorType type, char const* message, void* /* pUserData */){
		std::cout << "Uncapured device error: type " << type;
		if (message){ std::cout << "(" << message << ")"; }
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData*/);

	return device;
}

// the question is, does the lambdas still exist after this function gose out of scoop, do i need to bind there life times to the divce object?
WGPUDevice getNewDeviceAutoCallback(
	WGPUDeviceDescriptor& deviceDesc, 
	WGPUAdapter adapter
){
	deviceDesc.deviceLostCallbackInfo.nextInChain = nullptr;
	deviceDesc.deviceLostCallbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	//deviceDesc.deviceLostCallbackInfo.userdata; // i assume this is set by the callback function...

	deviceDesc.deviceLostCallbackInfo.callback = []( 
		WGPUDevice const * device,
		WGPUDeviceLostReason reason, 
		char const* message,
		void* /* pUserData */
	){
		std::cout << "Device lost: reason " << reason;
		if (message) { std::cout << "(" << message << ")";}
		std::cout << std::endl;

	};


	std::cout << "Request device..." << std::endl;

	WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);

	// we don't really need adapter after we got device(good practice to delete)
	// wgpuAdapterRelease(adapter);

	std::cout << "Got device: " << std::endl;

	/* Uncaptured Error Callback
		The uncaptured error callback is invoked whenever we misuse the API, and gives very informative feedback.
		It only set after the creation of the device, by calling wgpuDeviceSetUncapturedErrorCallback:
	*/
	auto onDeviceError = []( WGPUErrorType type, char const* message, void* /* pUserData */){
		std::cout << "Uncapured device error: type " << type;
		if (message){ std::cout << "(" << message << ")"; }
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData*/);

	return device;
}

////////////////////////////////////////////////////////////////////////////////////////
//// Queue
////////////////////////////////////////////////////////////////////////////////////////
WGPUQueue getQueue(WGPUDevice device){

	WGPUQueue queue = wgpuDeviceGetQueue(device);

	// set the callback that is executed when submitted work is done.
	auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2){
		std::cout << "Queued work finished with the status: " << status << std::endl;
	};

	WGPUQueueWorkDoneCallbackInfo2 callbackInfo = {}; // does the object need to live for the hoal life time of the queue?
	callbackInfo.nextInChain = nullptr;
	callbackInfo.mode = WGPUCallbackMode_AllowProcessEvents;
	callbackInfo.callback = onQueueWorkDone;
	
	//WGPUCallbackMode_AllowProcessEvents, polls the callback
	wgpuQueueOnSubmittedWorkDone2(queue, callbackInfo);

	return queue;
}

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
 ){
	WGPUSurfaceConfiguration config = {}; 
	config.nextInChain = nullptr;

	// configurations, should we have some configs as imput in appllications configs like window size...
	config.width = window_width;
	config.height = window_height;
	/*
	As you can guess, we will have to take care of re-configuring the surface when the 
	window is resized. In the meantime, do not try to resize it. 
	You may add glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); before creating the window 
	to instruct GLFW to disable resizing.
	*/

//All available combinations are listed in the WGPUTextureFormat enum, but since our swap chain targets an existing surface, we can just use whichever format the surface uses:
	WGPUSurfaceCapabilities surfaceCapabilities = {}; // can show different formats possible by the surface...
	surfaceCapabilities.nextInChain = nullptr;
	wgpuSurfaceGetCapabilities(surface, adapter, &surfaceCapabilities);

	///// telling how to allocate textures:
	//WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
	//config.format = surfaceFormat;
	config.format = surfaceCapabilities.formats[0];
	surfaceFormat = surfaceCapabilities.formats[0];
	// And we do not need any particular view format:
	config.viewFormatCount = 0;
	config.viewFormats = nullptr;
	
	// use the swap chain textures as targets for a Render Pass, so need to be created with this RenderAttachment flag:
	config.usage = WGPUTextureUsage_RenderAttachment;
	// Lastly, the surface needs to know the device to use to create the textures:
	config.device = device; 
	///// telling how to allocate textures:

	///// how tp ressent texture
	// Fifo: Stands for “first in, first out”, meaning that the presented texture is always the oldest one, like a regular queue. No rendered texture is wasted.
	config.presentMode = WGPUPresentMode_Fifo;

	//// we may specify how the textures will be composited onto the OS window, which may be used to create transparent windows.
	// left to auto_mode:
	config.alphaMode = WGPUCompositeAlphaMode_Auto;
	/*If you get the error Uncaptured device error: type 3 (Device(OutOfMemory)) when calling wgpuSurfaceConfigure, check that you specified the GLFW_NO_API value to glfw when creating the window.*/

	wgpuSurfaceConfigure(surface, &config);
}

////////////////////////////////////////////////////////////////////////////////////////
//// Asynchronous device polling, wgpuPollEvents
////////////////////////////////////////////////////////////////////////////////////////
/* //// Seams to be depricated:
// We define a function that hides implementation-specific variants of device polling:
void wgpuPollEvents( [[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser) {
	#ifdef WEBGPU_BACKEND_DAWN
		wgpuDeviceTick(device);
	#elif definde(WEBGPU_BACKEND_WGPU)
		wgpuDevicePoll(device, false, nullptr);
	#elif definde(WEBGPU_BACKEND_EMSCRIPTEN)
		if(yieldToWebBrowser){
			emscripten_sleep(100);
		}
	#endif
}
*/

////////////////////////////////////////////////////////////////////////////////////////
//// Renderpass
////////////////////////////////////////////////////////////////////////////////////////
WGPURenderPassColorAttachment getColorAttachment(WGPUTextureView targetView){
	// which textures are the target of this process. These are the attachments of the render pass.
	WGPURenderPassColorAttachment renderPassColorAttachment = {};

	// Describe the attachment
	/* 
	The first important setting of the attachment is the texture view it must draw in.
	In advanced pipelines it is very common to draw on intermediate textures,
	,which are then fed to e.g., post-processing passes.
	In our case, this is simply the targetView.
	*/
	renderPassColorAttachment.view = targetView;
	renderPassColorAttachment.resolveTarget = nullptr; // not use multi-sampling

	/*
	loadOp setting indicates the load operation to perform on the view prior to executing the render pass.
	It can be either read from the view or set to a default uniform color, namely the clear value. When it does not matter, use WGPULoadOp_Clear as it is likely more efficient.
	
	storeOp indicates the operation to perform on view after executing the render pass. 
	It can be either stored or discarded (the latter only makes sense if the render pass has side-effects).
	*/
	renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
	renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
	renderPassColorAttachment.clearValue = WGPUColor{ 0.05, 0.05, 0.05, 1.0 };//WGPUColor{ 0.9, 0.1, 0.2, 1.0 }; // the value to clear the screen

	// depthSlice to set in the attachment, that we must explicitly set to its undefined value because we do not use a depth buffer. 
	// This option is not supported by wgpu-native
	#ifndef WEBGPU_BACKEND_WGPU
		renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
	#endif // NOT WEBGPU_BACKEND_WGPU
	return renderPassColorAttachment;
}

WGPURenderPassEncoder encodeRenderPass(WGPUCommandEncoder encoder, WGPUTextureView targetView, WGPURenderPassTimestampWrites* timestampWrites){ // maby make a application class function
	// which textures are the target of this process. These are the attachments of the render pass.
	WGPURenderPassColorAttachment renderPassColorAttachment = getColorAttachment(targetView); // probably creats a copy of the WGPURenderPassColorAttachment created in getColorAttachment

	WGPURenderPassDescriptor renderPassDesc = {};
	renderPassDesc.nextInChain = nullptr;
	// this is one special type of attachment, namely the depth and stencil attachment(two chanels)
	renderPassDesc.depthStencilAttachment = nullptr;
	
	renderPassDesc.timestampWrites = timestampWrites; // When measuring the performance of a render pass
	
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;

	// Encode render pass, // A render pass leverages the 3D rendering circuits of the GPU to draw content into one or multiple textures.
	// WGPURenderPassEncoder renderPass = 
	
	return wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

	// use render pass, CommandBuffer need to be after this
	// wgpuRenderPassEncoderEnd(renderPass); 
	// wgpuRenderPassEncoderRelease(renderPass);
}

void setDefault(WGPUBindGroupLayoutEntry &bindingLayout) {
	/*
	The remaining of the binding layout depends on the type of resource. We 
	fill either the buffer field or the sampler and texture fields or the 
	storageTexture field. In our case it is a buffer:
	*/
    bindingLayout.buffer.nextInChain = nullptr;
    bindingLayout.buffer.type = WGPUBufferBindingType_Undefined;
    bindingLayout.buffer.hasDynamicOffset = false;

    bindingLayout.sampler.nextInChain = nullptr;
    bindingLayout.sampler.type = WGPUSamplerBindingType_Undefined;

    bindingLayout.storageTexture.nextInChain = nullptr;
    bindingLayout.storageTexture.access = WGPUStorageTextureAccess_Undefined;
    bindingLayout.storageTexture.format = WGPUTextureFormat_Undefined;
    bindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_Undefined;

    bindingLayout.texture.nextInChain = nullptr;
    bindingLayout.texture.multisampled = false;
    bindingLayout.texture.sampleType = WGPUTextureSampleType_Undefined;
    bindingLayout.texture.viewDimension = WGPUTextureViewDimension_Undefined;
}


void setupPipelineLayoutBindingGroup(
	WGPUDevice& device,
	WGPURenderPipelineDescriptor& pipelineDesc,
	WGPUPipelineLayout& layout,
	WGPUBindGroupLayout& bindGroupLayout,
	WGPUBindGroup& bindGroup,
	WGPUBuffer& uniformBuffer
){
	WGPUBindGroupLayoutEntry bindingLayout{}; // The BindGroupLayoutEntry could have been called BindingLayout.
	setDefault(bindingLayout);

	// The binding index as used in the @binding attribute in the shader
	bindingLayout.binding = 0;
	// The stage that needs to access this resource
	bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;//WGPUShaderStage_Vertex;
	bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(UniformStruct);

	// bind group layout
	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.nextInChain = nullptr;
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayout;
	bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

	// pipline layout
	WGPUPipelineLayoutDescriptor layoutDesc{};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &bindGroupLayout;
	// layoutDesc.label
	layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	pipelineDesc.layout = layout;
//// Pipeline layout end

///// Binding Group start, A bind group must follow the same structure as its layout
// The fields binding.sampler and binding.textureView are only needed when the binding layout uses them.
// Note that unlike the creation of the bind group layout, the creation of the bind group must come after the creation of the resources that it binds!
	WGPUBindGroupEntry binding{};
	binding.nextInChain = nullptr;
	// Setup binding
	// The index of the binding (the entries in bindGroupDesc can be in any order)
	binding.binding = 0;
	binding.buffer = uniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	binding.offset = 0;
	// And we specify again the size of the buffer.
	binding.size = sizeof(UniformStruct);

	// A bind group contains one or multiple bindings
	WGPUBindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.nextInChain = nullptr;
	bindGroupDesc.layout = bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &binding;
	bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc); 
///// Binding Group end
}

///
void setupPipelineLayoutMultipleBindingGroups(
	WGPUDevice& device,
	WGPURenderPipelineDescriptor& pipelineDesc,
	WGPUPipelineLayout& layout,
	WGPUBindGroupLayout& bindGroupLayout,
	WGPUBindGroup& bindGroup,
	WGPUBuffer& uniformBuffer,
	WGPUTextureView& textureView,
	WGPUSampler& sampler
){
	std::vector<WGPUBindGroupLayoutEntry> bindingLayoutEntries(3);
	
	WGPUBindGroupLayoutEntry& UniformBindingLayout = bindingLayoutEntries[0]; // The BindGroupLayoutEntry could have been called BindingLayout.
	setDefault(UniformBindingLayout);

	// The binding index as used in the @binding attribute in the shader
	UniformBindingLayout.binding = 0;
	// The stage that needs to access this resource
	UniformBindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;//WGPUShaderStage_Vertex;
	UniformBindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	UniformBindingLayout.buffer.minBindingSize = sizeof(UniformStruct);

	WGPUBindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1]; // The BindGroupLayoutEntry could have been called BindingLayout.
	setDefault(textureBindingLayout);
	textureBindingLayout.binding = 1;
	textureBindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
	textureBindingLayout.texture.sampleType = WGPUTextureSampleType_Float;
	textureBindingLayout.texture.viewDimension = WGPUTextureViewDimension_2D;

	// SAMPLER BindingLayout!!!!!!!!!!!!!!!
	WGPUBindGroupLayoutEntry& theSamplersBindingLayout = bindingLayoutEntries[2]; // The BindGroupLayoutEntry could have been called BindingLayout.
	setDefault(theSamplersBindingLayout);
	theSamplersBindingLayout.binding = 2;
	theSamplersBindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
	theSamplersBindingLayout.sampler.type = WGPUSamplerBindingType_Filtering;
	

	// bind group layout
	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.nextInChain = nullptr;
	bindGroupLayoutDesc.entryCount = static_cast<uint32_t>(bindingLayoutEntries.size());
	bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
	bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

	// pipline layout
	WGPUPipelineLayoutDescriptor layoutDesc{};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &bindGroupLayout;
	layoutDesc.label ="Three man bindingGroup";
	// layoutDesc.label
	layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

	pipelineDesc.layout = layout;
//// Pipeline layout end

///// Binding Group start, A bind group must follow the same structure as its layout
// The fields binding.sampler and binding.textureView are only needed when the binding layout uses them.
// Note that unlike the creation of the bind group layout, the creation of the bind group must come after the creation of the resources that it binds!
	std::vector<WGPUBindGroupEntry> bindings(3);
	bindings[0].nextInChain = nullptr;
	// Setup Uniform binding
	// The index of the binding (the entries in bindGroupDesc can be in any order)
	bindings[0].binding = 0;
	bindings[0].buffer = uniformBuffer;
	// We can specify an offset within the buffer, so that a single buffer can hold
	// multiple uniform blocks.
	bindings[0].offset = 0;
	// And we specify again the size of the buffer.
	bindings[0].size = sizeof(UniformStruct);

	// setup texture binding
	bindings[1].binding = 1;
	bindings[1].textureView = textureView;

	// setup sampler binding
	bindings[2].binding = 2;
	bindings[2].sampler = sampler;

	// A bind group contains one or multiple bindings
	WGPUBindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.nextInChain = nullptr;
	bindGroupDesc.layout = bindGroupLayout;
	// There must be as many bindings as declared in the layout!
	bindGroupDesc.entryCount = static_cast<uint32_t>(bindings.size());;
	bindGroupDesc.entries = bindings.data();
	bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc); 
///// Binding Group end
}

WGPURenderPipeline getVertexMultAttrRenderPipeline(
	WGPUDevice device, 
	WGPUTextureFormat& surfaceFormat,
	WGPUPipelineLayout& layout,
    WGPUBindGroupLayout& bindGroupLayout,
	WGPUBindGroup& bindGroup,
	WGPUBuffer& uniformBuffer,
	WGPUTextureView& textureView,
	WGPUSampler& sampler
){
	
	WGPURenderPipelineDescriptor pipelineDesc{};
	pipelineDesc.nextInChain = nullptr;

	// Vertex fetch
	WGPUVertexBufferLayout vertexBufferLayout{};
	// specify how many attributes it contains and detail each of them:
	//std::vector<WGPUVertexAttribute> vertexAttribs(2); // why use std::vector here? we know that we have only two attributes
	WGPUVertexAttribute vertexAttribs[2];
	// describe attribute

	// Describe the position attribute
	vertexAttribs[0].shaderLocation = 0; // @location(0)
	vertexAttribs[0].format = WGPUVertexFormat_Float32x2;
	vertexAttribs[0].offset = 0;
	
	// Describe the color attribute
	vertexAttribs[1].shaderLocation = 1; //  @location(1)
	vertexAttribs[1].format = WGPUVertexFormat_Float32x3;
	vertexAttribs[1].offset = 2 * sizeof(float); // offert by the size of the attribute before aka position
	
	vertexBufferLayout.arrayStride = 5 * sizeof(float); // 2 * sizeof(float);//
	vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

	vertexBufferLayout.attributeCount = 2;//sizeof(vertexAttribs);// 2;//static_cast<uint32_t>(vertexAttribs.size());
	vertexBufferLayout.attributes = vertexAttribs; // why &vertexAttribs not work?

	// Describe buffer stride and step mode
	pipelineDesc.vertex.nextInChain = nullptr;
	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	std::cout << "Creat shader module..." << std::endl;
	WGPUShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", device);//wgpuDeviceCreateShaderModule(device, &shaderDesc);
	//WGPUShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/raymarcher.wgsl", device);//wgpuDeviceCreateShaderModule(device, &shaderDesc);
	std::cout << "Shader module: " << shaderModule << std::endl;
	
	if(shaderModule == nullptr){
		std::cerr << "Shaders could not be loaded" << std::endl;
		exit(1);
	}

	
	pipelineDesc.vertex.module = shaderModule; // defined later
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

////// Primitive assembly aka razterization setup
	// configures the primitive assembly and rasterization stages.
	// The primitive assembly configuration consists in stating how the array of vertices fetched earlier must be connected to for either a point cloud, a wire frame or a triangle soup.
	pipelineDesc.primitive.nextInChain = nullptr;
	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	pipelineDesc.primitive.cullMode = WGPUCullMode_None; // WGPUCullMode_Front, means only rendering frontfacing triangles
////// Primitive assembly setup end

//// Fragment shader setup
	WGPUFragmentState fragmentState{};
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;
	
	pipelineDesc.fragment = &fragmentState; // this is optional, aka NULLABLE
//// Fragment shader setup end

//// Stencil/Depth state setup
	pipelineDesc.depthStencil = nullptr; // don't use the depth- or stencil-test
//// Stencil/Depth state setup end

//// Blending stage setup
// The blending stage takes each fragment’s color and “paints” it onto the target color attachment.
// (combines the "translucant" fragments to "paint" the pixel)
	WGPUBlendState blendState{};
	// Configure color blending equation, rgb = srcFactor * rgb_s [operation] dstFactor * rgb_d
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	// Configure alpha blending equation, Target alpha untouched: a = a_d = 0 * a_s + 1 * a_d
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;

	WGPUColorTargetState colorTarget{};
	colorTarget.format = surfaceFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
	// We have only one target because our render pass has only one output color attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
//// Blending stage setup end

//// Multisanpling setup
/*I said previously that a fragment is the portion of a primitive that is projected onto a specific pixel. Actually, we can split pixels into sub-elements, called samples, and the fragment is associated to a sample. The value of a pixel is computed by averaging its associated samples.*/
	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meang "all bits on"
	pipelineDesc.multisample.mask = ~0u; // usigned integer
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;
//// Multisanpling setup end
	
//// Pipeline layout
	/*
	- Shaders might need to access input and output resources (buffers and/or textures). These resources are made available to the pipeline by configuring a memory layout.
	- Setting the pipeline layout to nullptr does not mean that there is no input/output resources. It rather asks the backend to figure out the layout by itself by inspecting the shader (which in this very case is equivalent).
	*/
	// creat the layout for the Uniforms used in the shader:
	// binding layout
	/*
	setupPipelineLayoutBindingGroup(
		device,
		pipelineDesc,
		layout,
		bindGroupLayout,
		bindGroup,
		uniformBuffer
	);
	*/
	setupPipelineLayoutMultipleBindingGroups(
		device,
		pipelineDesc,
		layout,
		bindGroupLayout,
		bindGroup,
		uniformBuffer,
	 	textureView,
		sampler
	);

///// Binding Group end

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);

	wgpuShaderModuleRelease(shaderModule); // dont need this after useage...

	return pipeline;
}

void drawSquareWithIndexBuffer(
	WGPURenderPipeline pipline, 
	WGPURenderPassEncoder renderPass, 
	WGPUBuffer& pointBuffer,
	WGPUBuffer& indexBuffer,
	uint32_t indexCount
){
	// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderPass, pipline);

	// Set vertex buffer while encoding the render pass
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, pointBuffer, 0, wgpuBufferGetSize(pointBuffer));

	wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(indexBuffer));
	// We use the `vertexCount` variable instead of hard-coding the vertex count
	wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0); // with multiple(aka 2) instances, we get an instance index 0u and 1u
}

void drawSquareWithIndexNUniformBuffers(
	WGPURenderPipeline pipline, 
	WGPURenderPassEncoder renderPass, 
	WGPUBuffer& pointBuffer,
	WGPUBuffer& indexBuffer,
	uint32_t indexCount,
	WGPUBindGroup bindGroup
){

	// Select which render pipeline to use
	wgpuRenderPassEncoderSetPipeline(renderPass, pipline);
	wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, pointBuffer, 0, wgpuBufferGetSize(pointBuffer));
	wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(indexBuffer));

	// Set bindnig group here
	wgpuRenderPassEncoderSetBindGroup(renderPass, 0, bindGroup, 0, nullptr);

	// We use the `vertexCount` variable instead of hard-coding the vertex count
	wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0); // with multiple(aka 2) instances, we get an instance index 0u and 1u
}


////////////////////////////////////////////////////////////////////////////////////////
//// Load ShaderModule from file in resources, fully copyed......
////////////////////////////////////////////////////////////////////////////////////////
WGPUShaderModule loadShaderModule(
	const std::filesystem::path& path, 
	WGPUDevice device
){
	std::ifstream file(path);
	if (!file.is_open()){
		return nullptr;
	}
	file.seekg(0, std::ios::end); // jump to end of file, offset 0 to endof file std::ios::end
	size_t size = file.tellg(); // get the posision value of the last "char"
	std::string shaderSource(size, ' '); // creat a empty sting of the same number of characters as the file
	file.seekg(0); // go back to the files start
	file.read(shaderSource.data(), size); // read the hoal file in to the string
	
	WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	shaderCodeDesc.code = shaderSource.c_str();

	WGPUShaderModuleDescriptor shaderDesc{};
	shaderDesc.nextInChain = nullptr;
	#ifdef WEBGPU_BACKEND_WGPU // what is these used fore again...
		shaderDesc.hintCount = 0;
		shaderDesc.hints = nullptr;
	#endif
		shaderDesc.nextInChain = &shaderCodeDesc.chain;
		return wgpuDeviceCreateShaderModule(device, &shaderDesc);
}

// could write a secound loadShaderModule function that could load Spir-V?

////////////////////////////////////////////////////////////////////////////////////////
//// Uniforms and bind Groups
////////////////////////////////////////////////////////////////////////////////////////

/*
	1. Declare the uniform in the shader.

	2. Create the buffer it is bound to.

	3. Configure the properties of the binding (a.k.a. the binding’s layout).

	4. Create a bind group.
*/


////////////////////////////////////////////////////////////////////////////////////////
//// Application
////////////////////////////////////////////////////////////////////////////////////////
bool Application::Initialize(){ // this seams a bit old school...

	ready = false; // set to true after buffer mapping for timestamps
	numberOfQuerySets = 8; // start with four because uint64_t => 8 bytes per timestamp, 2 timestamps per query => 2*8 = 16, 4 querySets => 16*4 = 64 bytes witch is average size of L1 cache line
	querySetCounter = 0;

	if(!glfwInit()){
		std::cerr << "Could not initilice GLFW" << std::endl;
		return false;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// shaderToy resolution: 800 x 450 
	window_width = 800;// 480;//800;//1600;;//640.0;
	window_height = 450;//480;//800;//450;//900;//640.0;//480.0;

	window = glfwCreateWindow(window_width, window_height, "Learn WebGPU", nullptr, nullptr);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		glfwTerminate();
		return false;
	}
	
	instance = {};
	WGPUInstanceDescriptor desc = {};
	desc.nextInChain = nullptr;
	getInstance(desc, instance); // returns false if no instance could be created...
	/*
	WGPUInstanceDescriptor desc ={};
	desc.nextInChain = nullptr;
	if(!getInstance(desc,instance)){
		return false;
	}
	*/
	//WGPUAdapter adapter = getAdapter();
	// probably need to check that the surface is correctly created aka not nullptr
	surface = glfwGetWGPUSurface(instance, window); // cross (OS)platform get surface(different for each)
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	adapter = getAdapter(instance, adapterOpts);
	// getAdapterLimits(adapter);

	WGPUDeviceDescriptor deviceDesc = {};  
	WGPURequiredLimits limits = setRequredLimits();
	size_t featureCount = 2;
	WGPUFeatureName featutes[2] = {WGPUFeatureName_TimestampQuery, WGPUFeatureName_Float32Filterable};

	// we should make a function that checks for the feature and tells oss if the adapter/device has it and then we can use that ifo to stop  using it...
	setDeviceDescriptor(deviceDesc, limits, featureCount, featutes);
	//device = getDefaultDevice(adapter);
	device = getNewDeviceAutoCallback(deviceDesc, adapter);
	//if(!device){ return false; } // there should be a callback that handles errors
	//inspectDevice(device);

	queue = getQueue(device);

	// should this be application class function? Maby not, only if we have a specific application... but then it could be a subclass and a virtual function... Also could be private...
	configureSurfaceSetup(device, adapter, surface, surfaceFormat, window_width, window_height);

	//InitializeUniformBuffers(); // need to be done before bindGroup is initialized.
	InitializeUniformStructBuffer();
	//pipeline = getVertexAttrRenderPipeline(device, surfaceFormat);//getStandardPipeline(device, surfaceFormat);
	InitializeTextureSampler();
	
	//grayNoiseView = InitializeGrayNoiseTexture(); // sets the texture view for grayNoise texture
	ComputePassCreatTexture();
	
	pipeline = getVertexMultAttrRenderPipeline(
		device, 
		surfaceFormat,
		layout, 
		bindGroupLayout, 
		bindGroup, 
		uniformBuffer, 
		grayNoiseView, 
		sampler
	);

// Vertex buffer data initialized and sent to GPU. 
	//InitializeMultiAttBuffers();
	//InitializeIndexBuffers();
	InitializeConemarchIndexBuffers();
	InitializeTimestampBuffer();
//// initialize querySet for time-stamps
	WGPUQuerySetDescriptor querySetDesc{};
	querySetDesc.nextInChain = nullptr;
	querySetDesc.type = WGPUQueryType_Timestamp;
	querySetDesc.label = "RenderPass timestamp";
	querySetDesc.count = 2*static_cast<uint32_t>(numberOfQuerySets);//2; // one for the start of renderpass and one for the end/GPU-work done 
	timestampQuerySet = wgpuDeviceCreateQuerySet(device, &querySetDesc);

	std::cout << "Surface format: " << surfaceFormat << std::endl;

	return true;
}

// I don't like having the main loop mostly defined away from the main function... Could i just move it's definition to main.cpp?
void Application::MainLoop(){
	//std::chrono::high_resolution_clock cpuClock;

	auto cpuTimeStart = std::chrono::steady_clock::now();
	glfwPollEvents();
	// [...] Main loop content

	// Update uniform buffer
	UniformStruct inputUniforms;
	inputUniforms.iResolution = {static_cast<float>(window_width), static_cast<float>(window_height)};
	double xpos = 0; double ypos = 0;
	glfwGetCursorPos(window, &xpos, &ypos);
	xpos += window_width;
	ypos += window_height;
	inputUniforms.iMouse ={(float)(xpos/100.0),(float)(ypos/100.0)};
	inputUniforms.iTime = static_cast<float>(glfwGetTime());
	//float t = static_cast<float>(glfwGetTime()); // glfwGetTime() returns a double
	wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &inputUniforms, sizeof(UniformStruct));

	// Get the next target texture view
	auto [surfaceTexture, targetView] = getNextSurfaceViewData();
	if (!targetView) return; // should we give error message?? // you're not releasing anything because the returned texture/view should not be considered valid in that failure path???

// Draw things start
	WGPUCommandEncoderDescriptor encoderDesc = {}; 
	encoderDesc.nextInChain = nullptr; 
	encoderDesc.label = "My command encoder";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

//// Setting up querySet's for timeStamps used to meshure the start and end of a render pass, need TimeStamp_query feature	
	bool reachedTimestampLimit = ( ((querySetCounter/2)+1) > numberOfQuerySets ); // querySetCounter = {0,2,5,6  , 8} om numberOfQuerySets = 4
	
	WGPURenderPassEncoder renderPass{};
	if(!reachedTimestampLimit){ // then we should not set a new querySet...
		WGPURenderPassTimestampWrites timestampWrites{};
		timestampWrites.querySet = timestampQuerySet; // do we need a new querySet for each time stamp...
		timestampWrites.beginningOfPassWriteIndex = static_cast<uint32_t>(querySetCounter);//0;
		timestampWrites.endOfPassWriteIndex = ++querySetCounter;//1;
		++querySetCounter; // 

		// Encode the renderpass
		renderPass = encodeRenderPass(encoder, targetView, &timestampWrites);
	} else {
		renderPass = encodeRenderPass(encoder, targetView, nullptr);
	}

	// render triangles
	//drawSquareWithIndexBuffer(pipeline, renderPass, pointBuffer, indexBuffer, indexCount);
	drawSquareWithIndexNUniformBuffers(pipeline, renderPass, pointBuffer, indexBuffer, indexCount, bindGroup);


	// use render pass, CommandBuffer need to be after this
	wgpuRenderPassEncoderEnd(renderPass); 
	wgpuRenderPassEncoderRelease(renderPass);

//////////////////////////// Should we "queue" the 
bool allTimestampsAreQueued = ( querySetCounter == (numberOfQuerySets*2)) ;

	if(allTimestampsAreQueued){
		const uint32_t firstQuery = 0;//querySetCounter-2;//0;// firstQuery → where to start reading from the query set
		const uint32_t queryCount = numberOfQuerySets * 2;
		const uint64_t destinationOffset = 0;// numberOfQuerySets * 2 * sizeof(uint64_t);// (querySetCounter-2) * sizeof(uint64_t);//destinationOffset → where to start writing into the destination buffer
		wgpuCommandEncoderResolveQuerySet(encoder, timestampQuerySet, firstQuery, queryCount, timestampBuffer, destinationOffset);
	
		const uint64_t sourceOffset = 0;
		//const uint64_t destinationOffset = 0;
		const uint64_t timestapBufferSize = numberOfQuerySets * 2 * sizeof(uint64_t);//2; // does  this need to be a multiple of 4...
		wgpuCommandEncoderCopyBufferToBuffer(encoder, timestampBuffer, sourceOffset, mappedTimeBuffer, destinationOffset, timestapBufferSize); // we could count how many wgpuCommandEncoderResolveQuerySet we have called and only compy the hoal timestampBuffer when it's full..... Maby waste fullllllll
	}
////////////////////////////////
	/////////////// CommandBuffer, send the encoder's command to the GPU
	WGPUCommandBufferDescriptor cmdDesc = {};
	WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &cmdDesc);
	wgpuQueueSubmit(queue, 1, &commandBuffer);
	///////////////

	wgpuCommandEncoderRelease(encoder); // should this be a class varible?
	wgpuCommandBufferRelease(commandBuffer);

	// check error for command queue, atleast we only get one status print, but shold be called each time loop is repeted... But does lead to Device lost: reason 3(A valid external Instance reference no longer exists.)
	// wgpuInstanceProcessEvents(instance); is the new modern webGPU way that poll's events that is now instance specific instead of device.
	//wgpuInstanceProcessEvents(instance); // this should only be called when there is something to poll...
	//wgpuPollEvents(device, true /* yieldToBrowser */);
// Drawing end

if(allTimestampsAreQueued) {
///// Mapping work need to be queued after the work on the buffer is queued aka after wgpuQueueSubmit(...);
	// Should this mapping happen in the initilization faze...
	// define callback, (how do i use the massage feald? could be usefull when reusing callbacks)
	auto onBuffer2Mapped = [](WGPUMapAsyncStatus status, const char* message, void* userdata1, void* userdata2 ) {
		// What we should do is print once when success and then only print if this command then fails at any point..., but avoid the last one where instance is destroyed...
		
		// We know by convention with ourselves that the user data is a pointer to 'ready':
		bool* pReady = reinterpret_cast<bool*>(userdata1); 
		if(!*pReady){ // if pReady is already true then we know that the code has already been executed, stops reapeted eronies error messages
			*pReady = true; // race condition problems if multiple callbacks are supposed to effect this varible?
			
			std::cout << "mappedTimeBuffer mapped with status " << status << std::endl;
			if(status != WGPUMapAsyncStatus_Success) 
				std::cout << " and message: " << message << std::endl; // error/status message
		}
	};

	WGPUBufferMapCallbackInfo2 bufferCallBackInfo;
	bufferCallBackInfo.nextInChain = nullptr;
	bufferCallBackInfo.mode = WGPUCallbackMode_AllowProcessEvents; // we have to activly call polling?
	bufferCallBackInfo.userdata1 = (void*)&ready;
	bufferCallBackInfo.userdata2 = nullptr;//(void*)mappedTimeBuffer;//nullptr;//(void*)&buffer2;
	bufferCallBackInfo.callback = onBuffer2Mapped;

	// This function does: "Map this when it is safe"
	wgpuBufferMapAsync2(mappedTimeBuffer, WGPUMapMode_Read, 0, 2*numberOfQuerySets, bufferCallBackInfo);
	querySetCounter++; // so we don't repeat
}

////// End of launching acync mapp work

	// In emscripten we rather rely on  emscripten_set_main_loop_arg (a.k.a. requestAnimationFrame in JavaScript) to call our MainLoop() function right before presenting.
	#ifndef __EMSCRIPTEN__ // we must not call wgpuSurfacePresent() 
		// "tells WebGPU" that the current surface texture is ready to be presented to the window/compositor
		wgpuSurfacePresent(surface); // must be called after the render pass has ended
	#endif // NOT __EMSCRIPTEN__

	// We no longer need the texture, after we get the view but:
    // (NB: with wgpu-native, surface textures must be release after the call to wgpuSurfacePresent)
    wgpuTextureRelease(surfaceTexture.texture);
	// Release the texture at end of the frame
	wgpuTextureViewRelease(targetView);
	//bool whaitingForCallBack = true;
	//while((querySetCounter>8) && !ready){ // if ready is set to true then we have processed the mapping call back
	wgpuInstanceProcessEvents(instance); // i don't think we need to continuly check this...
	//}
	if (ready){
		auto cpuTimeEnd = std::chrono::steady_clock::now();
		const uint64_t* TimestampData = reinterpret_cast<const uint64_t*>(wgpuBufferGetConstMappedRange(mappedTimeBuffer, 0, 2*numberOfQuerySets));
		uint64_t GPUTimeSum = 0;
		uint64_t GPUTime = 0;
		const uint64_t nd = 2*numberOfQuerySets;
		for(int i=0; i < nd; i++){
			GPUTime = TimestampData[i+1] - TimestampData[i];
			GPUTimeSum += GPUTime; 
			//std::cout <<"frame: "<< i/2 << " | " << GPUTime/1e6<< "ms" << std::endl;
			i++;
		}
		GPUTimeSum /= numberOfQuerySets;
		std::cout << "average of 4 frames: " <<  GPUTimeSum/1e6<< "ms" << " = " << 1e9/GPUTimeSum << " fps" << std::endl;
		wgpuBufferUnmap(mappedTimeBuffer);
		//auto cpuTimeEnd = std::chrono::steady_clock::now();
		std::chrono::nanoseconds cpuElapsedTime = cpuTimeEnd - cpuTimeStart;
		uint64_t elapsedNanoseconds = cpuElapsedTime.count();
		std::cout << "CPU time Spent untill Callback divided by 8 = " << (elapsedNanoseconds/1e6)/nd << "ms" << " = " << 1.0/(((elapsedNanoseconds)/(1e9))/nd) << " fps" << std::endl;
		querySetCounter = 0;
		ready = false;
	}
}

bool Application::IsRunning(){
	return !glfwWindowShouldClose(window);
}

void Application::Terminate(){

	// we need to both destroy and release a texture?
	wgpuTextureDestroy(grayNoise);
	wgpuTextureRelease(grayNoise);

	//wgpuTextureViewDestroy(grayNoiseView);
	wgpuTextureViewRelease(grayNoiseView);

// computePass cleanup:
	wgpuComputePipelineRelease(computePipeline);
// End of computePass cleanup

	wgpuBufferRelease(pointBuffer);
	wgpuBufferRelease(indexBuffer);
	wgpuBufferRelease(vertexBuffer);
	
	wgpuBufferRelease(uniformBuffer);
	wgpuPipelineLayoutRelease(layout);
	wgpuBindGroupLayoutRelease(bindGroupLayout);
	wgpuBindGroupRelease(bindGroup);
	
	wgpuBufferRelease(timestampBuffer);
	wgpuQuerySetRelease(timestampQuerySet);
	
	wgpuRenderPipelineRelease(pipeline);
	wgpuQueueRelease(queue);
	
	wgpuDeviceRelease(device);
	wgpuAdapterRelease(adapter);

	wgpuSurfaceUnconfigure(surface);
	wgpuSurfaceRelease(surface);

	wgpuInstanceRelease(instance);

	glfwDestroyWindow(window);
	glfwTerminate();
}

std::pair<WGPUSurfaceTexture, WGPUTextureView> Application::getNextSurfaceViewData(){
	/* Get the next surface texture
	To get the texture to draw onto, we use wgpuSurfaceGetCurrentTexture. 
	The “surface texture” is not really an object but rather a container for 
	the multiple things that this function returns.
	*/
	WGPUSurfaceTexture surfaceTexture{};
	wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
	if(surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success){
		std::cout << "SurfaceGetCurrentTexture failed with status: " << surfaceTexture.status << std::endl;
		return {surfaceTexture, nullptr };
	} else if(surfaceTexture.suboptimal){ // if surfaceTexture.suboptimal is true then the Texture is suboptimal...
		std::cout << "SurfaceGetCurrentTextureStatus Succeeded but is suboptimal according to surfaceTexture.suboptimal's WGPUBool value" << std::endl;
	}

	/* Create surface texture view
	What we will need in the next section is not directly the surface texture,
	but a texture view, which may represent a sub-part of the texture, or expose 
	it using a different format.
	*/
	WGPUTextureViewDescriptor viewDescriptor;
	viewDescriptor.nextInChain = nullptr;
	viewDescriptor.label = "Surface texture view";
	viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
	viewDescriptor.dimension = WGPUTextureViewDimension_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = WGPUTextureAspect_All;
	WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

	// Release the texture
	return { surfaceTexture, targetView };
}

WGPURequiredLimits Application::setRequredLimits(/*WGPUAdapter adapter*/) const{ // this const, more or less implize (object pointer:) MyClass* const this; aka this can't be modified
	// Get adapter supported limits, in case we need them
	WGPUSupportedLimits supportedLimits;
	supportedLimits.nextInChain = nullptr;
	wgpuAdapterGetLimits(adapter, &supportedLimits);

	WGPURequiredLimits requiredLimits{};
	setDefaultLimits(requiredLimits.limits);
	// These two limits are different because they are "minimum" limits,
	// they are the only ones we may forward from the adapter's supported
	// limits.
	requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
	requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

	// We use at most 1 vertex attribute for now
	requiredLimits.limits.maxVertexAttributes = 2; //1;<= not multi attributes
	// We should also tell that we use 1 vertex buffers
	requiredLimits.limits.maxVertexBuffers = 1;

	/* WE SHOULD CHANGE THE BLIMITS.MAXBUFFER SIZE DEPENDING ON IF WE DO SUBDIVISON!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	uint16_t quadSize = 32;
	uint16_t vertexSize = quadSize + 1;

	std::vector<uint16_t> indexData;
	indexData.reserve(quadSize*quadSize * 6 *2);
	*/
	constexpr uint32_t NumberOfQuads = 64;
    constexpr uint32_t vertexSize = NumberOfQuads + 1;

	// Maximum size of a buffer is 6 vertices of 2 float each
	requiredLimits.limits.maxBufferSize = vertexSize * vertexSize * 6 * sizeof(float) ;//6 * 5 * sizeof(float); ;//6 * 2 * sizeof(float); // <= one attrib
	// Maximum stride between 2 consecutive vertices in the vertex buffer, (why?)
	requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);// 2*sizeof(float); // <= one attrib
	// [...] other device limitations'

	// for the multi attributes, because There is a limit on the number of components that can be forwarded from vertex to fragment shader.
	// in multi attributes There is a maximum of 3 float forwarded from vertex to fragment shader
	requiredLimits.limits.maxInterStageShaderComponents = 7;// 3;<= this is wrong because @builtin(0) position: vec4f and @location(0) color: vec3f, is forwarded

	// Limits associated with Uniorms aka bindgroups:
	// We use at most 1 bind group for now
	requiredLimits.limits.maxBindGroups = 3;
	// We use at most 1 uniform buffer per stage
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	// Uniform structs have a size of maximum 16 float (more than what we need)
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;

	// Add the possibility to sample a texture in a shader
	requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
	requiredLimits.limits.maxSamplersPerShaderStage = 1;

	requiredLimits.limits.maxStorageTexturesPerShaderStage = 1;
/*
maxStorageTexturesPerShaderStage
 uint32_t maxComputeWorkgroupStorageSize;
    uint32_t maxComputeInvocationsPerWorkgroup;
    uint32_t maxComputeWorkgroupSizeX;
    uint32_t maxComputeWorkgroupSizeY;
    uint32_t maxComputeWorkgroupSizeZ;
    uint32_t maxComputeWorkgroupsPerDimension;
*/

	return requiredLimits;
}

void Application::InitializeIndexBuffers(){
	// Define point data
	// The de-duplicated list of point positions
	std::vector<float> pointData = {
    // x,   y,     r,   g,   b
    -1.0, -1.0,   1.0, 0.0, 0.0,
    +1.0, -1.0,   0.0, 1.0, 0.0,
    +1.0, +1.0,   0.0, 0.0, 1.0,
    -1.0, +1.0,   1.0, 1.0, 0.0
	};
	vertexCount = static_cast<uint32_t>(pointData.size() / 5);

	// Creat vertex buffer
	WGPUBufferDescriptor bufferDesc{}; // if this one is not inizilized it will fuck things up,(if even one feald is not initialized)
	bufferDesc.nextInChain = nullptr;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // vertex usage here
	bufferDesc.mappedAtCreation = false; // do not make contents accessible to CPU, through a CPU pointer/range..
	pointBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, pointBuffer, 0, pointData.data(), bufferDesc.size);

	//Define index data
	// This is a list of indices referencing positions in the pointData
	// can use uint16_t or uint32_t, just depends on how manny vertices we want to limit oss to for memory savings
	std::vector<uint16_t> indexData = { 
		0, 1, 2, // Triangle #0 connects points #0, #1 and #2
    	0, 2, 3  // Triangle #1 connects points #0, #2 and #3
	};

	indexCount = static_cast<uint32_t>(indexData.size()); // indexCount should be how many indecies aka indexData.size() / 2

	// Creat index buffer
	//WGPUBufferDescriptor bufferDesc{}; // if this one is not inizilized it will fuck things up,(if even one feald is not initialized)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	/* WHY IS THIS??
	A writeBuffer operation must copy a number of bytes that is a multiple of 4. To ensure this, we must 
	ceil the buffer size up to the next multiple of 4 before creating it:
	*/
	bufferDesc.size = (bufferDesc.size + 3 ) & ~3; // round up to the next multiple of 4, roundup number of bytes..
	// This means that we must also make sure that indexData.size() is a multiple of 2 (because sizeof(uint16_t) is 2):
	indexData.resize((indexData.size() + 1) & ~1); // round up to the next multiple of 2, 2 => 2*16bits => 2*2bytes => 4bytes, this is padding for alignment? Roundup number of indexes/vertice, 

	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
	indexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, indexBuffer, 0, indexData.data(), bufferDesc.size);
}

/*
std::vector<float> creatConeMarchPointData(){
	uint16_t quadSize = 32;
	uint16_t vertexSize = quadSize + 1;
	std::vector<float> pointData;
	pointData.reserve(vertexSize * vertexSize *2); // *2 to avoid allocating once after all filled
	std::cout << "Start of vertex creation:" << std::endl;
	for(double y=1; y > -1; y -= 0.0625)
		for(double x=-1; x < 1; x += 0.0625){
			pointData.emplace_back(static_cast<float>(x));
			pointData.emplace_back(static_cast<float>(y));
		}
	std::cout << "End of vertex creation " << std::endl;
	return pointData;
}
*/
std::vector<float> creatConeMarchPointData()
{
    constexpr uint32_t NumberOfQuads = 32;
    constexpr uint32_t vertexSize = NumberOfQuads + 1;

    std::vector<float> pointData;
    pointData.reserve(vertexSize * vertexSize * 5 * 2); // *2 to avoid allocating once after all filled

    for (uint32_t y = 0; y <= NumberOfQuads; ++y)
    {
        float fy = 1.0f - (static_cast<float>(y) / NumberOfQuads) * 2.0f;

        for (uint32_t x = 0; x <= NumberOfQuads; ++x)
        {
            float fx = -1.0f + (static_cast<float>(x) / NumberOfQuads) * 2.0f;

			pointData.push_back(fx);
			pointData.push_back(fy);

			if(y % 2 == 0){
				if( x % 2 == 0){
					pointData.push_back(1.0);
            		pointData.push_back(0.0);
            		pointData.push_back(0.0);
				} else {
					pointData.push_back(0.0);
            		pointData.push_back(1.0);
            		pointData.push_back(0.0);
				}
			} else {
				if( x % 2 == 0){
					pointData.push_back(0.0);
            		pointData.push_back(0.0);
            		pointData.push_back(1.0);
				} else {
					pointData.push_back(1.0);
            		pointData.push_back(1.0);
            		pointData.push_back(0.0);
				}
			}


        }
    }
    return pointData;
}

std::vector<uint16_t> creatConeMarchIndexData(){

	uint16_t quadSize = 32;
	uint16_t vertexSize = quadSize + 1;

	std::vector<uint16_t> indexData;
	indexData.reserve(quadSize*quadSize * 6 *2); // *2 to avoid allocating once after all filled

	uint16_t x0, x1,  x2, x3 = {};

	std::cout << "Start of test-indeces creation:" << std::endl;
	for(uint16_t y=0; y < quadSize; y++)
		for(uint16_t x=0; x < quadSize; x++){

			x0 = y*vertexSize + x;
			x1 = y*vertexSize + (x+1);
			x2 = (y+1)*vertexSize + (x+1);
			x3 = (y+1)*vertexSize + x;

			indexData.emplace_back(x0);
			indexData.emplace_back(x1);
			indexData.emplace_back(x2);

			indexData.emplace_back(x0);
			indexData.emplace_back(x2);
			indexData.emplace_back(x3);
			/*
			std::cout << "x0 = " << x0 << std::endl;
			std::cout << "x1 = " << x1<< std::endl;
			std::cout << "x2 = " << x2 << std::endl;
			std::cout << "x3 = " << x3<< std::endl;
			std::cout << "triangle one = ( " << x0 << ", " << x1 << ", " << x2 << " )" <<std::endl;
			std::cout << "triangle two = ( " << x0 << ", " << x2 << ", " << x3 << " )" <<std::endl;
			*/
		}

	std::cout << "End of test-indeces creation " << std::endl;
	return indexData;
}

void Application::InitializeConemarchIndexBuffers(){

	std::vector<float> pointData = creatConeMarchPointData();

	vertexCount = static_cast<uint32_t>(pointData.size() / 2);

	// Creat vertex buffer
	WGPUBufferDescriptor bufferDesc{}; // if this one is not inizilized it will fuck things up,(if even one feald is not initialized)
	bufferDesc.nextInChain = nullptr;
	bufferDesc.size = pointData.size() * sizeof(float);
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex; // vertex usage here
	bufferDesc.mappedAtCreation = false; // do not make contents accessible to CPU, through a CPU pointer/range..
	pointBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, pointBuffer, 0, pointData.data(), bufferDesc.size);

	//Define index data
	// This is a list of indices referencing positions in the pointData
	// can use uint16_t or uint32_t, just depends on how manny vertices we want to limit oss to for memory savings
	std::vector<uint16_t> indexData = creatConeMarchIndexData();

	indexCount = static_cast<uint32_t>(indexData.size()); // indexCount should be how many indecies aka indexData.size() / 2

	// Creat index buffer
	//WGPUBufferDescriptor bufferDesc{}; // if this one is not inizilized it will fuck things up,(if even one feald is not initialized)
	bufferDesc.size = indexData.size() * sizeof(uint16_t);
	/* WHY IS THIS??
	A writeBuffer operation must copy a number of bytes that is a multiple of 4. To ensure this, we must 
	ceil the buffer size up to the next multiple of 4 before creating it:
	*/
	bufferDesc.size = (bufferDesc.size + 3 ) & ~3; // round up to the next multiple of 4, roundup number of bytes..
	// This means that we must also make sure that indexData.size() is a multiple of 2 (because sizeof(uint16_t) is 2):
	indexData.resize((indexData.size() + 1) & ~1); // round up to the next multiple of 2, 2 => 2*16bits => 2*2bytes => 4bytes, this is padding for alignment? Roundup number of indexes/vertice, 

	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index; // vertex usage here
	indexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, indexBuffer, 0, indexData.data(), bufferDesc.size);
}

void Application::InitializeUniformStructBuffer(){
	WGPUBufferDescriptor bufferDesc{};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.mappedAtCreation = false;

	bufferDesc.size = sizeof(UniformStruct); // uniform buffer needs to be aligned to 16 bytes
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform; // WGPUBufferUsage_CopyDst => CPU to GPU uppload for buffer?

	uniformBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
}

void Application::InitializeTimestampBuffer(){
	WGPUBufferDescriptor bufferDesc{};
	bufferDesc.nextInChain = nullptr;
	bufferDesc.mappedAtCreation = false; // does this one need to be mapped

	// the buffer probably need to be aligned to 8 bytes because whats in resolveQuerySet: Set offset to be offset + 8.
	bufferDesc.size = 2 * numberOfQuerySets * sizeof(uint64_t); // uniform buffer needs to be aligned to 16 bytes
	//bufferDesc.size = (bufferDesc.size + 3 ) & ~3; // roundup to the nearest multiple of foure
	bufferDesc.label = "timestampBuffer";
	// WGPUBufferUsage_CopySrc: GPU can copy from, Aka the source
	bufferDesc.usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_QueryResolve;
	timestampBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);	

	bufferDesc.label = "mappedTimeBuffer";
	// WGPUBufferUsage_CopyDst: GPU can copy to this buffer, Aka the destination
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead; 
	//bufferDesc.mappedAtCreation = true; // how does it work when mapped at creation? does it just exclude the callback function?
	mappedTimeBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

} // WGPUBufferUsage_CopyDst

float get_random_noize_corner(const float i, const float j) {
	const float iScaled = i*123.34f;
	const float jScaled = j*456.21f;

	const float pi = iScaled - std::floorf(iScaled);
	const float pj = jScaled - std::floorf(jScaled);
	
	const float pDot = pi * (pi + 45.32f) + pj * (pj + 45.32f);

	const float piDot = pi + pDot;
	const float pjDot = pj + pDot;
	const float dotMdot = piDot*pjDot;
	const float fractVal = dotMdot - floorf(dotMdot);
	return fractVal;//(fractVal*2.0f) - 1.0f;
	/*
    let p = fract(ij * vec2f(123.34, 456.21));
    let p2 = p + dot(p, p + 45.32);
    return fract(p2.x * p2.y) * 2.0 - 1.0;
	*/
}

// initializes and uploads the noize texture to the GPU
WGPUTextureView Application::InitializeGrayNoiseTexture(){

	WGPUTextureDescriptor textureDesc{};
	textureDesc.nextInChain = nullptr;
	textureDesc.label = " graynoise";

	textureDesc.dimension = WGPUTextureDimension_2D;
	textureDesc.size = { 256, 256, 1 };
	//                             ^ ignored because it is a 2D texture
	textureDesc.mipLevelCount = 1;
	textureDesc.sampleCount = 1; // numbers of samples for multisampling stuff
	/*Both the mip level count and the sample count have an impact on the memory 
	size of the texture, so when not using these features, setting them to 
	(the minimum) saves memory.*/

	textureDesc.format = WGPUTextureFormat_R8Unorm; // this usage: | WGPUTextureUsage_StorageBinding; is incompatibale withWGPUTextureFormat_R8Unorm format, might f32 aka float might just be ok
	textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst; // sampling it from a shader | copy pixel data from CPU 
	
	// Lastly, we initialize the texture view settings to 0 (more on this later):
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;

	grayNoise = wgpuDeviceCreateTexture(device, &textureDesc);
	
	// Creat noise data
	std::vector<uint8_t> pixels(textureDesc.size.width * textureDesc.size.height); //{}
	for(uint32_t j = 0; j < textureDesc.size.height; j++)
		for(uint32_t i = 0; i < textureDesc.size.width; i++){
			float noise = get_random_noize_corner((float)i,(float)j);//* 0.5f + 0.5f; // the noise is [-1, 1]
			pixels[ j * textureDesc.size.width + i] = static_cast<uint8_t>(noise * 255.0f + 0.5f); // we add 0.5 så that the noise values that have a decimal value over zero rounds up
		}


	// Arguments telling which part of the texture we upload to
	// (together with the last argument of writeTexture)
	WGPUImageCopyTexture destination{};
	destination.texture = grayNoise;
	destination.mipLevel = 0; // The writeTexture procedure writes only one image (subresource of the texture) at a time. The argument destination.mipLevel tells which one we target, and in our case we have only one mip level.
	destination.origin = {0,0,0}; // equivalent of the offset argument of Queue::writeBuffer
	destination.aspect = WGPUTextureAspect_All; // only relevant for depth/Stencil textures

	// Arguments telling how the C++ side pixel memory is laid out
	WGPUTextureDataLayout source{};
	source.offset = 0;
	source.bytesPerRow = 1 * textureDesc.size.width;
	source.rowsPerImage = textureDesc.size.height;
	//source.nextInChain = nullptr; // why not set this now?
	/*Note:
	When loading data from an image file, rows might be aligned to a round number of bytes, hence source.bytesPerRow would be slightly bigger than 4 * textureDesc.size.width. This is also needed when uploading very small images because there is a minimum value for bytesPerRow that is set to 256 by the API.	
	*/

	// Write aka uppload the texture to the GPU(this one does not need queueSubmit function, it's scheduald by it self)
	wgpuQueueWriteTexture(queue, &destination, pixels.data(), pixels.size(), &source, &textureDesc.size);	

	// 
	WGPUTextureViewDescriptor textureViewDesc{};
	textureViewDesc.aspect = WGPUTextureAspect_All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.dimension = WGPUTextureViewDimension_2D;
	textureViewDesc.format = textureDesc.format;
	return wgpuTextureCreateView(grayNoise, &textureViewDesc);;
}

void Application::InitializeTextureSampler(){
	WGPUSamplerDescriptor samplerDesc{};
	samplerDesc.nextInChain =nullptr;
	samplerDesc.addressModeU = WGPUAddressMode_Repeat;
	samplerDesc.addressModeV = WGPUAddressMode_Repeat;
	samplerDesc.addressModeW = WGPUAddressMode_Repeat; // third dimension not needed...
samplerDesc.compare = WGPUCompareFunction_Undefined;
samplerDesc.label ="first Sampler";
samplerDesc.lodMaxClamp = 1.0f; // WHY THIS
samplerDesc.lodMinClamp = 0.0f; // WHY THIS

samplerDesc.magFilter = WGPUFilterMode_Linear; // // WHY THIS
samplerDesc.minFilter = WGPUFilterMode_Linear;
samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
samplerDesc.maxAnisotropy = 1; // WHY THIS


 sampler = wgpuDeviceCreateSampler(device, &samplerDesc);
}


WGPUTextureView Application::CreatEmptyGrayNoiseTexture(){

	WGPUTextureDescriptor textureDesc{};
	textureDesc.nextInChain = nullptr;
	textureDesc.label = " graynoise";

	textureDesc.dimension = WGPUTextureDimension_2D;
	textureDesc.size = { 256, 256, 1 };//{ 512, 512, 1 };
	//                             ^ ignored because it is a 2D texture
	textureDesc.mipLevelCount = 1;
	textureDesc.sampleCount = 1; // numbers of samples for multisampling stuff
	/*Both the mip level count and the sample count have an impact on the memory 
	size of the texture, so when not using these features, setting them to 
	(the minimum) saves memory.*/

	textureDesc.format = WGPUTextureFormat_R32Float; // this usage: | WGPUTextureUsage_StorageBinding; is incompatibale withWGPUTextureFormat_R8Unorm format, might f32 aka float might just be ok
	// I need to set the feature WGPUTextureFormat_R32Float if i want to use linear filter(sampling) stufff
	
	textureDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_StorageBinding; // sampling it from a shader | copy pixel data from CPU 
	
	// Lastly, we initialize the texture view settings to 0 (more on this later):
	textureDesc.viewFormatCount = 0;
	textureDesc.viewFormats = nullptr;

	grayNoise = wgpuDeviceCreateTexture(device, &textureDesc);

	WGPUTextureViewDescriptor textureViewDesc{};
	textureViewDesc.aspect = WGPUTextureAspect_All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = 1;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = 1;
	textureViewDesc.dimension = WGPUTextureViewDimension_2D;
	textureViewDesc.format = textureDesc.format;
	return wgpuTextureCreateView(grayNoise, &textureViewDesc);;
}

void Application::ComputePassCreatTexture(){

	WGPUCommandEncoderDescriptor encoderDesc = {}; 
	encoderDesc.nextInChain = nullptr; 
	encoderDesc.label = "Encoder for the compute pass";
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
	
	// DON'T forget to dubble check limitaions and features that might be needed ...
	// creat compute pass

// initialize computePipline:

// BindGroupLayout:
	WGPUBindGroupLayoutEntry textureBindingLayout{}; // The BindGroupLayoutEntry could have been called BindingLayout.
	setDefault(textureBindingLayout);
	textureBindingLayout.binding = 0;
	/*
	textureBindingLayout.visibility = WGPUShaderStage_Compute;
	textureBindingLayout.texture.sampleType = WGPUTextureSampleType_Float;
	textureBindingLayout.texture.viewDimension = WGPUTextureViewDimension_2D;
	*/
	textureBindingLayout.visibility = WGPUShaderStage_Compute;
	textureBindingLayout.storageTexture.access = WGPUStorageTextureAccess_WriteOnly;
	textureBindingLayout.storageTexture.format = WGPUTextureFormat_R32Float;
	textureBindingLayout.storageTexture.viewDimension = WGPUTextureViewDimension_2D;

	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.nextInChain = nullptr;
	bindGroupLayoutDesc.label = "Noise texture bindgrouplayout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &textureBindingLayout;
	computeBindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);
// End BindGroupLayout

// BindGroup:
	grayNoiseView = CreatEmptyGrayNoiseTexture(); // theres nothing about this name that sugest the return of a textureView
	
	WGPUBindGroupEntry TextureBinding{};
	TextureBinding.binding = 0;
	TextureBinding.textureView = grayNoiseView;

	WGPUBindGroupDescriptor bindGroupDesc{};
	bindGroupDesc.label = "bindingGroup layout";
	bindGroupDesc.nextInChain = nullptr;
	bindGroupDesc.layout = computeBindGroupLayout;
	bindGroupDesc.entryCount = 1;
	bindGroupDesc.entries = &TextureBinding;
	computeBindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc); 

// End bindGroup

// PipelineLayout:
WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
pipelineLayoutDesc.nextInChain = nullptr;
pipelineLayoutDesc.label ="Compute piplineLayout";
pipelineLayoutDesc.bindGroupLayoutCount = 1;
pipelineLayoutDesc.bindGroupLayouts = &computeBindGroupLayout;
computeLayout = wgpuDeviceCreatePipelineLayout(device, &pipelineLayoutDesc);
// End PipelineLayout

	WGPUShaderModule computeShaderModule = loadShaderModule(RESOURCE_DIR "/computeShader.wgsl", device);

	WGPUComputePipelineDescriptor computePiplineDesc{};
	computePiplineDesc.nextInChain = nullptr; // this is not one of those where this need to be not nullptr...
	computePiplineDesc.label = "Noise compute shader pipeline";
	computePiplineDesc.compute.entryPoint = "cs_main";
	computePiplineDesc.compute.module = computeShaderModule;
	computePiplineDesc.layout = computeLayout; // we should need to set this
	computePiplineDesc.compute.nextInChain = nullptr;
	computePipeline = wgpuDeviceCreateComputePipeline(device, &computePiplineDesc);
// End initialize computePipline

	WGPUComputePassDescriptor computePassDesc{};
	computePassDesc.nextInChain = nullptr;
	computePassDesc.label = "computepass";
	computePassDesc.timestampWrites = nullptr;
	WGPUComputePassEncoder computePass = wgpuCommandEncoderBeginComputePass(encoder,&computePassDesc);

	wgpuComputePassEncoderSetPipeline(computePass, computePipeline);
	wgpuComputePassEncoderSetBindGroup(computePass, 0, computeBindGroup, 0, nullptr);

	/*
	uint32_t invocationCountX = m_textureSize.width;
	uint32_t invocationCountY = m_textureSize.height;
	*/
	//uint32_t invocationCount = 512;
	uint32_t invocationCount = 256;
	uint32_t workgroupSizePerDim = 4;
	// This ceils invocationCountX / workgroupSizePerDim
	uint32_t workgroupCount = (invocationCount + workgroupSizePerDim - 1) / workgroupSizePerDim;
	
	wgpuComputePassEncoderDispatchWorkgroups(computePass, workgroupCount, workgroupCount,1);//16,16,1);

	wgpuComputePassEncoderEnd(computePass);
	// Finalize compute pass
	//computePass.end();


	WGPUCommandBufferDescriptor commandDesc{};
	WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &commandDesc);
	wgpuQueueSubmit(queue, 1, &command);

	// local cleanup of creat rescources
	wgpuComputePassEncoderRelease(computePass);
	wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(encoder);
}