// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

class VulkanEngine {
public:

	bool _isInitialized{ false };
	int _frameNumber {0};
	bool stop_rendering{ false };
	VkExtent2D _windowExtent{ 1440 , 900 };

	VkInstance _instance; // Vulkan instance handle
	VkDebugUtilsMessengerEXT _debug_messenger; // Vulkan debug output handle
	VkPhysicalDevice _chosenGPU; // GPU chosen to be used as default device
	VkDevice _device; // Vulkan logical device for commands
	VkSurfaceKHR _surface; // Vulkan window surface
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;  // Actual image objects to render into
	std::vector<VkImageView> _swapchainImageViews; // Wrappers for these images
	VkExtent2D _swapchainExtent;

	struct SDL_Window* _window{ nullptr };

	static VulkanEngine& Get();

	// Initializes everything in the engine
	void init();

	// Initializes vulkan instance
	void init_vulkan();

	// Initializes swapchain
	void init_swapchain();

	// Initializes command buffer
	void init_commands();

	// Initialises synchronisation structures
	void init_sync_structures();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	// We need to rebuild swapchain as window resizes, so we have them separated from init_swapchain();
	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();
};
