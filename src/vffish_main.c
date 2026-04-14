#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <volk/volk.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Application {
		VkInstance instance;
}Application;

bool check(VkResult result) {
	if(result == VK_ERROR_INITIALIZATION_FAILED) {
		printf("Failed to initialize vulkan module, code: %d", result);
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
		
		check(volkInitialize());
		
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
			
			free(extensionProperties);
		}
		
		VkApplicationInfo appInfo;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "vffish";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
		appInfo.apiVersion = VK_API_VERSION_1_3;
		
		VkInstanceCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = extensionCount;
		createInfo.ppEnabledExtensionNames = extensionNames;
		
		Application *app = malloc(sizeof(Application));
		
		// Remember to free *app, *extensionNames;
	
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}