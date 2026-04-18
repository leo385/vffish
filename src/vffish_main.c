#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <vulkan/vulkan.h>
#include <Volk/volk.h>
#include <VMA/vk_mem_alloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

typedef struct Application {
		VkInstance instance;
}Application;

bool check(VkResult result) {
	if(result == VK_ERROR_INITIALIZATION_FAILED) {
		printf("Failed to initialize vulkan module, code: %d\n", result);
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
	
	return 0;
}

int main(int argv, char** argc) {

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("vffish", 600u, 400u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	Application *app = malloc(sizeof(Application));
	
    bool quit = false;

    while(!quit) {
        for(SDL_Event event; SDL_PollEvent(&event);) {
            if(event.type == SDL_EVENT_QUIT) {
                quit = true;
                break;
            }
        }
		
		if(volkInitialize() != VK_SUCCESS) {
			return 1;
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
		
		/*
			Ensure Vk structures at the beginning are initialized with { 0 },
			because vkCreateInstance is looking for structures with *pNext, and then garbage
			could be loaded into VkInstance, that may cause SEGMENTATION FAULT.
		*/
		VkApplicationInfo appInfo = { 0 };
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vffish";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_3;
		
		VkInstanceCreateInfo createInfo = { 0 };
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		/* MoltenVK may generate VK_ERROR_INCOMPATIBILE_DRIVER, that's why
		   we want to prevent this error by checking VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR extension */
		#ifdef __APPLE__
			createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		#endif
		
		createInfo.enabledExtensionCount = currentExtensionCount;
		createInfo.ppEnabledExtensionNames = extensionNames;
		
		VkResult result;
		if(app) {
			check(result = vkCreateInstance(&createInfo, NULL, &app->instance));
		}
		
		/* extensionProperties are no more needed due to instance was already created */
		free(extensionProperties);
		free(extensionNames);
		if(app && result == VK_SUCCESS) volkLoadInstance(app->instance);

		/* Count devices needed for rendering */
		uint32_t currentDevicesCount = 0;
		VkPhysicalDevice* physicalDevices = NULL;
		
		if(app) {
			check(result = vkEnumeratePhysicalDevices(app->instance, &currentDevicesCount, NULL));
			
			if(currentDevicesCount == 0) {
				printf("There are not available devices for rendering");
				return 1;
			}
			
			printf("Available devices: %d\n", currentDevicesCount);
			physicalDevices = malloc(currentDevicesCount * sizeof(VkPhysicalDevice));
			check(result = vkEnumeratePhysicalDevices(app->instance, &currentDevicesCount, physicalDevices));
		}
		
		/* Provide device index to the console */
		uint32_t deviceIndex = { 0 };
		if(argv > 1) {
			const char* cDeviceIndex = argc[1];
			char *endptr;
			
			errno = 0;
			unsigned long index = strtoul(cDeviceIndex, &endptr, 10);
			if((uint32_t)index >= currentDevicesCount) {
				printf("This device index doesn't exist\n");
				return 1;
			}
			
			if(errno == ERANGE || (uint32_t)index > UINT_MAX) {
				perror("This index is too big\n");
				return 1;
			}
			
			deviceIndex = (uint32_t)index;
		}
		
		/* Display device information with choosen index */
		if(physicalDevices) {
			VkPhysicalDeviceProperties2 deviceProperties = { 0 };
			deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(physicalDevices[deviceIndex], &deviceProperties);
			printf("Selected device: %s\n", deviceProperties.properties.deviceName);
		}
	
    }

	/* Cleanup vulkan resources */
	if(app) {
		vkDestroyInstance(app->instance, NULL);
		free(app);
	}
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}