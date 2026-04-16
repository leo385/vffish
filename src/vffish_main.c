#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <vulkan/vulkan.h>
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(void) {

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("vffish", 600u, 400u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
		
		VkExtensionProperties *extensionProperties = malloc(extensionCount * sizeof(VkExtensionProperties));
		const char** extensionNames = malloc(extensionCount * sizeof(const char*));
		
		if(extensionProperties != NULL) {
			
			vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionProperties);
			printf("%d extensions have been found of vulkan\n", extensionCount);
			
			for(uint32_t i = 0; i < extensionCount; i++) {
				
				printf("Extension name: %s\n", extensionProperties[i].extensionName);
				extensionNames[i] = extensionProperties[i].extensionName;
			}
			
		}
		
		/*
			Ensure Vk structures at the beginning are initialized with { 0 },
			because vkCreateInstance is looking for structures, and then garbage
			could be loaded into VkInstance, that may cause SEGMENTATION FAULT.
		*/
		VkApplicationInfo appInfo = {0};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vffish";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_3;
		
		VkInstanceCreateInfo createInfo = {0};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensionNames;
		
		Application *app = malloc(sizeof(Application));
		VkResult result;
		if(app) {
				check(result = vkCreateInstance(&createInfo, NULL, &app->instance));
		}
		
		/* extensionProperties are no more needed due to instance was created */
		free(extensionProperties);
		free(extensionNames);
		if(app && result == VK_SUCCESS) volkLoadInstance(app->instance);
		
		// Remember to free *app, *extensionNames;
		free(app);
	
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}