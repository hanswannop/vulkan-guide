﻿//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

// Vulkan bootstrap library, can remove later setup done from scratch using Vulkan tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code
#include "VkBootstrap.h"

#include <chrono>
#include <thread>

constexpr bool bUseValidationLayers = true;

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loadedEngine == nullptr);
    loadedEngine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

	init_vulkan();

	init_swapchain();

	init_commands();

	init_sync_structures();

    // everything went fine
    _isInitialized = true;
}

void VulkanEngine::init_vulkan()
{
	// Use VkBootsrap to make a Vulkan 1.3 instance 
	// with basic debug features active.
	// TODO: read vulkan tutorial and do this manually
	vkb::InstanceBuilder builder;

	auto inst_ret = builder.set_app_name("Vulkan Engine")
	.request_validation_layers(bUseValidationLayers)
	.use_default_debug_messenger()
	.require_api_version(1, 3, 0)
	.build();

	vkb::Instance vkb_inst = inst_ret.value();

	// Get the resulting instance from Vulkan bootstrap
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	// Create the VkSurfaceKHR object for the SDL window
	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	// Select vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	// Select vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12 { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	// Select the physical device with our chosen features using vkbootstrap that can render to our surface
	vkb::PhysicalDeviceSelector selector { vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
	.set_minimum_version(1, 3)
	.set_required_features_13(features13)
	.set_required_features_12(features12)
	.set_surface(_surface)
	.select()
	.value();

	// Make the final vulkan device
	vkb::DeviceBuilder deviceBuilder { physicalDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the handles from the vkbootsrap builders
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder { _chosenGPU, _device, _surface };
	_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	
	vkb::Swapchain vkbSwapchain = swapchainBuilder
	.set_desired_format(VkSurfaceFormatKHR { .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
	.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)  // Hard vsync, limit fps of engine to vsync
	.set_desired_extent(width,height)
	.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	.build()
	.value();

	_swapchainExtent = vkbSwapchain.extent;

	// Store swapchain and its images
	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain()
{
	// Delete swapchain, this will also delete the images it holds
	vkDestroySwapchainKHR(_device, _swapchain, nullptr);

	// Delete the ImageViews associated
	for (int i = 0; i < _swapchainImageViews.size(); i++) {
		vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
	}
}

void VulkanEngine::init_swapchain() 
{
	// Create swapchain with default size as the window size
	create_swapchain(_windowExtent.width, _windowExtent.height); 
}

void VulkanEngine::init_commands()
{
}

void VulkanEngine::init_sync_structures()
{
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {
		destroy_swapchain();
		vkDestroyInstance(_instance, nullptr);
		vkDestroyDevice(_device, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		// Note, we dont destroy VKPhysical device, its not a Vulkan resource, just a handle to the GPU

		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		
        SDL_DestroyWindow(_window);
    }

    // clear engine pointer
    loadedEngine = nullptr;
}

void VulkanEngine::draw()
{
    // nothing yet
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool bQuit = false;

    // main loop
    while (!bQuit) {
		// Handle events on queue
		while (SDL_PollEvent(&e) != 0) {
			// close the window when user alt-f4s or clicks the X button
			if (e.type == SDL_QUIT)
				bQuit = true;
			if (e.type == SDL_KEYDOWN) {
				fmt::print("Key pressed: {}\n", SDL_GetKeyName(e.key.keysym.sym));
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					bQuit = true;
				}
			}
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        // do not draw if we are minimized
        if (stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}