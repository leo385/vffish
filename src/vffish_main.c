#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_vulkan.h>
#include <Volk/volk.h>
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 1
#define VMA_ASSERT(expr) assert(expr)
#include <VMA/vk_mem_alloc.h>
#include <vulkan/vulkan_beta.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

typedef struct Window {
	SDL_Window *window;
	ivec2 size;
}Window;

typedef struct Application {
		VkInstance instance;
		VkDevice device;
		VkQueue queue;
		VmaAllocator allocator;
		VkSurfaceKHR surface;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkSwapchainKHR swapchain;
		VkImage* swapchainImages;
		VkImage depthImage;
		VkImageView depthImageView;
		VkImageView* swapchainImageViews;
		VkFormat* depthFormatList;

		/* VMA allocation for depth image */
		VmaAllocation depthImageAllocation;
}Application;

/* Error handling */
static inline int check(VkResult result) {
	if(result == VK_ERROR_INITIALIZATION_FAILED) {
		printf("Error: Failed to initialize vulkan module, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_LAYER_NOT_PRESENT) {
		printf("Error: Layer not presented, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_EXTENSION_NOT_PRESENT) {
		printf("Error: Extension not presented, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_SURFACE_LOST_KHR) {
		printf("Error: Lost surface, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_OUT_OF_HOST_MEMORY) {
		printf("Error: Out of host memory, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
		printf("Error: Out of device memory, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_UNKNOWN) {
		printf("Error: Unknown, code: %d\n", result);
		return 1;
	}
	else if(result == VK_ERROR_VALIDATION_FAILED) {
		printf("Error: Validation failed, code: %d\n", result);
		return 1;
	}
	
	return 0;
}

int main(int argv, char** argc) {

	uint8_t exit_code = 0;

    if(!SDL_Init(SDL_INIT_VIDEO)) {
		exit_code = 1;
		goto cleanup;
    }

	if(volkInitialize() != VK_SUCCESS) {
		exit_code = 1;
		goto cleanup;
	}

	Application *sApp = malloc(sizeof(Application));
	if(!sApp) {
		exit_code = 1;
		goto cleanup;
	}
	
	// Vulkan code
	uint32_t currentExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &currentExtensionCount, NULL);
	
	VkExtensionProperties *extensionProperties = malloc(currentExtensionCount * sizeof(VkExtensionProperties));
	const char** extensionNames = malloc((currentExtensionCount + ALLOCATION_OPTIONAL_EXTENSION) * sizeof(const char*));
	
	if(extensionProperties != NULL) {
		
		vkEnumerateInstanceExtensionProperties(NULL, &currentExtensionCount, extensionProperties);
		printf("%d extensions have been found of vulkan\n", currentExtensionCount);
		
		for(uint32_t i = 0; i < currentExtensionCount; i++) {
			
			printf("Extension name: %s\n", extensionProperties[i].extensionName);
			extensionNames[i] = extensionProperties[i].extensionName;
			
			/* Allocating for optional extension */
			#ifdef __APPLE__
				extensionNames[i+ALLOCATION_OPTIONAL_EXTENSION] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
			#endif
		}
		
	}
	else{
		exit_code = 1;
		goto cleanup;
	}
	
	/*
		Ensure Vk structures at the beginning are initialized with { 0 },
		because vkCreateInstance is looking for structures with *pNext, and then garbage
		could be loaded into VkInstance, that may cause SEGMENTATION FAULT.
	*/
	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "vffish",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_API_VERSION_1_3
	};
	
	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledExtensionCount = currentExtensionCount,
		.ppEnabledExtensionNames = extensionNames
	};

	/* MoltenVK may generate VK_ERROR_INCOMPATIBILE_DRIVER, that's why
		we want to prevent this error by checking VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR extension 
	*/
	#ifdef __APPLE__
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	#endif
	
	VkResult result;
	check(result = vkCreateInstance(&createInfo, NULL, &sApp->instance));
	
	/* extensionProperties are no more needed due to instance was already created */
	free(extensionProperties);
	free(extensionNames);
	if(result == VK_SUCCESS) volkLoadInstance(sApp->instance);

	/* Count devices needed for rendering */
	uint32_t currentDevicesCount = 0;
	
	check(result = vkEnumeratePhysicalDevices(sApp->instance, &currentDevicesCount, NULL));
	
	if(currentDevicesCount == 0) {
		printf("There are not available devices for rendering");
		exit_code = 1;
		goto cleanup;
	}
	
	printf("Available devices: %d\n", currentDevicesCount);
	VkPhysicalDevice* physicalDevices = malloc(currentDevicesCount * sizeof(VkPhysicalDevice));
	
	if(!physicalDevices) {
		exit_code = 1;
		goto cleanup;
	}

	check(result = vkEnumeratePhysicalDevices(sApp->instance, &currentDevicesCount, physicalDevices));
	
	/* Provide device index to the console */
	uint32_t deviceIndex = { 0 };
	if(argv > 1) {
		const char* cDeviceIndex = argc[1];
		char *endptr;
		
		errno = 0;
		unsigned long index = strtoul(cDeviceIndex, &endptr, 10);
		if((uint32_t)index >= currentDevicesCount) {
			printf("This device index doesn't exist\n");
			exit_code = 1;
			goto cleanup;
		}
		
		if(errno == ERANGE || (uint32_t)index > UINT_MAX) {
			perror("This index is too big\n");
			exit_code = 1;
			goto cleanup;
		}
		
		deviceIndex = (uint32_t)index;
	}
	
	/* Display device information with choosen index */
	VkPhysicalDeviceProperties2 deviceProperties = { 0 };
	deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	vkGetPhysicalDeviceProperties2(physicalDevices[deviceIndex], &deviceProperties);
	printf("Selected device: %s\n", deviceProperties.properties.deviceName);

	/* Select queue family for graphics operation due to later rendering something on the screen */
	uint32_t queueFamilyCount = { 0 };
	VkQueueFamilyProperties2 queueFamilyProperties2 = { 0 };
	queueFamilyProperties2.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;

	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[deviceIndex], &queueFamilyCount, &queueFamilyProperties2);
	VkQueueFamilyProperties2* queueFamilyProperties = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties2));
	vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[deviceIndex], &queueFamilyCount, queueFamilyProperties);
	uint32_t queueFamily = { 0 };
	for(size_t i = 0; i < queueFamilyCount; ++i) {
		if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamily = i;
			break;
		}
	}

	/* Check if queue family for presentation graphics is supported on the device */
	if(!SDL_Vulkan_GetPresentationSupport(sApp->instance, physicalDevices[deviceIndex], queueFamily)) {
		printf("VK_Queue_GRAPHICS_BIT is unsupported on this device");
		exit_code = 1;
		goto cleanup;
	}

	const float qfPriorities = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qfPriorities
	};

	/* Setup device */
	const char** deviceExtensions = malloc(2 * sizeof(const char*));
	deviceExtensions[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	deviceExtensions[1] = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;

	/* Enable extensions from API 1.0 to 1.3 (baseline) */
	VkPhysicalDeviceFeatures enabledVk10Features = {
		.samplerAnisotropy = VK_TRUE
	};

	VkPhysicalDeviceVulkan12Features enabledVk12Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.descriptorIndexing = true,
		.shaderSampledImageArrayNonUniformIndexing = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray = true,
		.bufferDeviceAddress = true
	};

	VkPhysicalDeviceVulkan13Features enabledVk13Features = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk12Features,
		.synchronization2 = true,
		.dynamicRendering = true
	};

	/* Create logical device with all artifacts */
	VkDeviceCreateInfo deviceCreateInfo = { 
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &enabledVk13Features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = deviceExtensions,
		.pEnabledFeatures = &enabledVk10Features
	};

	check(vkCreateDevice(physicalDevices[deviceIndex], &deviceCreateInfo, NULL, &sApp->device));
	/* Query a device for a queue to submit graphics commands into */
	vkGetDeviceQueue(sApp->device, queueFamily, 0, &sApp->queue);
	
	/* VMA Settings */
	VmaVulkanFunctions vkFunctions = {
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
		.vkCreateImage = vkCreateImage
	};

	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = physicalDevices[deviceIndex],
		.device = sApp->device,
		.pVulkanFunctions = &vkFunctions,
		.instance = sApp->instance
	};

	check(vmaCreateAllocator(&allocatorCreateInfo, &sApp->allocator));

	/* Window and surface */
	Window *sWindow = malloc(sizeof(Window));
	if(!sWindow) {
		exit_code = 1;
		goto cleanup;
	}

    sWindow->window = SDL_CreateWindow("vffish", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	if(!sWindow->window) {
		printf("Failed to create a window: %s\n", SDL_GetError());
		exit_code = 1;
		goto cleanup;
	}

	if(!SDL_GetWindowSize(sWindow->window, &sWindow->size[0], &sWindow->size[1])) {
		printf("Failed to get window size: %s\n", SDL_GetError());
		exit_code = 1;
		goto cleanup;
	}
	const uint32_t windowSizeX = sWindow->size[0];
	const uint32_t windowSizeY = sWindow->size[1];

	if(!SDL_Vulkan_CreateSurface(sWindow->window, sApp->instance, NULL, &sApp->surface)){
		printf("Failed to create vulkan surface.\n");
		exit_code = 1;
		goto cleanup;
	}

	/* Store surface capabilities for reference in future */
	check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[deviceIndex], sApp->surface, &sApp->surfaceCapabilities));

	VkExtent2D swapchainExtent = { 
		.width = sApp->surfaceCapabilities.currentExtent.width,
		.height = sApp->surfaceCapabilities.currentExtent.height 
	};

	/* On Wayland, surface capabilities hold width as 0xFFFFFFFF value */
	if(sApp->surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
		swapchainExtent.width = windowSizeX;
		swapchainExtent.height = windowSizeY;
	}

	const VkFormat imageFormat = { VK_FORMAT_B8G8R8A8_SRGB };
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = sApp->surface,
		.minImageCount = sApp->surfaceCapabilities.minImageCount,
		.imageFormat = imageFormat,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = { .width = swapchainExtent.width, .height = swapchainExtent.height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};
	check(vkCreateSwapchainKHR(sApp->device, &swapchainCreateInfo, NULL, &sApp->swapchain));

	/* Get swapchain images */
	uint32_t imagesCount = { 0 };
	check(vkGetSwapchainImagesKHR(sApp->device, sApp->swapchain, &imagesCount, NULL));
	sApp->swapchainImages = malloc(imagesCount * sizeof(VkImage));
	check(vkGetSwapchainImagesKHR(sApp->device, sApp->swapchain, &imagesCount, sApp->swapchainImages));
	sApp->swapchainImageViews = malloc(imagesCount * sizeof(VkImageView));
	for(size_t i = 0; i < imagesCount; ++i) {
		VkImageViewCreateInfo viewCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = sApp->swapchainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = imageFormat,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
		};
		check(vkCreateImageView(sApp->device, &viewCreateInfo, NULL, &sApp->swapchainImageViews[i]));
	}

	/* Depth attachment */
	const size_t depthFormatSizeList = 2;
	sApp->depthFormatList = malloc(depthFormatSizeList * sizeof(VkFormat));
	sApp->depthFormatList[0] = VK_FORMAT_D32_SFLOAT_S8_UINT;
	sApp->depthFormatList[1] = VK_FORMAT_D24_UNORM_S8_UINT;
	VkFormat depthFormat = { VK_FORMAT_UNDEFINED };

	for(size_t i = 0; i < depthFormatSizeList; ++i) {
		VkFormatProperties2 formatProperties = { .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
		vkGetPhysicalDeviceFormatProperties2(physicalDevices[deviceIndex], sApp->depthFormatList[i], &formatProperties);
		if(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = sApp->depthFormatList[i];
			break;
		}
	}

	VkImageCreateInfo depthImageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depthFormat,
		.extent = { .width = windowSizeX, .height = windowSizeY, .depth = 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocCI = {
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};
	check(vmaCreateImage(sApp->allocator, &depthImageCreateInfo, &allocCI, &sApp->depthImage, &sApp->depthImageAllocation, NULL));
	
	VkImageViewCreateInfo depthImageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = sApp->depthImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = depthFormat,
		.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 }
	};
	check(vkCreateImageView(sApp->device, &depthImageViewCreateInfo, NULL, &sApp->depthImageView));

    bool quit = false;
    while(!quit) {
        for(SDL_Event event; SDL_PollEvent(&event);) {
            if(event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }
        }
    }

	cleanup:
	/* Cleanup vulkan resources */
	if(sApp) {
		if(sApp->depthFormatList) {
			free(sApp->depthFormatList);
		}

		if(sApp->depthImageView) {
			vkDestroyImageView(sApp->device, sApp->depthImageView, NULL);
		}

		if(sApp->depthImage) {
			vmaDestroyImage(sApp->allocator, sApp->depthImage, sApp->depthImageAllocation);
		}

		if(sApp->allocator) {
			vmaDestroyAllocator(sApp->allocator);
		}

		if(sApp->swapchainImageViews){
			for(size_t i = 0; i < imagesCount; ++i) {
				vkDestroyImageView(sApp->device, sApp->swapchainImageViews[i], NULL);
			}
		}

		if(sApp->swapchain) {
			vkDestroySwapchainKHR(sApp->device, sApp->swapchain, NULL);
		}

		free(sApp->swapchainImageViews);
		free(sApp->swapchainImages);

		if(physicalDevices){
			free(physicalDevices);
		}

		if(queueFamilyProperties){
			free(queueFamilyProperties);
		}

		if(deviceExtensions){
			free(deviceExtensions);
		}

		if(sApp->surface) {
			SDL_Vulkan_DestroySurface(sApp->instance, sApp->surface, NULL);
		}

		vkDestroyDevice(sApp->device, NULL);
		vkDestroyInstance(sApp->instance, NULL);
		free(sApp);

		return exit_code;
	}
	if(sWindow) {
    	SDL_DestroyWindow(sWindow->window);
		free(sWindow);
	}

    SDL_Quit();
    
    return exit_code;
}