#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_events.h>
#include <vulkan/vulkan.h>
#include <stdio.h>

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
		
		// Vulkan code
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
		printf("%d extensions have been found of vulkan\n", extensionCount);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}