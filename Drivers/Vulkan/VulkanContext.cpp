/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 Red Gogh All rights reserved.                         *|
|*                                                                                  *|
|*    Licensed under the Apache License, Version 2.0 (the "License");               *|
|*    you may not use this file except in compliance with the License.              *|
|*    You may obtain a copy of the License at                                       *|
|*                                                                                  *|
|*        http://www.apache.org/licenses/LICENSE-2.0                                *|
|*                                                                                  *|
|*    Unless required by applicable law or agreed to in writing, software           *|
|*    distributed under the License is distributed on an "AS IS" BASIS,             *|
|*    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.      *|
|*    See the License for the specific language governing permissions and           *|
|*    limitations under the License.                                                *|
|*                                                                                  *|
\* -------------------------------------------------------------------------------- */
#include "VulkanContext.h"

#ifdef USE_VOLK_LOADER
#  define VOLK_IMPLEMENTATION
#  include <volk/volk.h>
#endif

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#ifdef WIN32
#include <vulkan/vulkan_win32.h>
#endif

VulkanContext::VulkanContext(const Window *_window) : window(_window)
{
    VkResult err;
    uint32_t count;

    // -------------- //
    // -- Instance -- //
    // -------------- //
    
    /*
     * initialize volk loader to dynamic load about instance function
     * api pointer.
     */
    err = volkInitialize();
    VK_CHECK_FATAL_ERROR("Failed to initialize volk loader", err);

    err = vkEnumerateInstanceVersion(&apiVersion);
    VK_CHECK_FATAL_ERROR("Can't not get instance version", err);

    printf("Vulkan %u.%u.%u\n",
           VK_VERSION_MAJOR(apiVersion),
           VK_VERSION_MINOR(apiVersion),
           VK_VERSION_PATCH(apiVersion));

    VkApplicationInfo info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Veronak Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Veronak Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = apiVersion
    };

    std::vector<const char *> extensions;

    const char **required = glfwGetRequiredInstanceExtensions(&count);
    for (int i = 0; i < count; ++i)
        extensions.push_back(required[i]);

    std::vector<const char *> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instance_ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &info,
        .enabledLayerCount = (uint32_t) std::size(layers),
        .ppEnabledLayerNames = std::data(layers),
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),

    };

    err = vkCreateInstance(&instance_ci, VK_NULL_HANDLE, &instance);
    VK_CHECK_FATAL_ERROR("Failed to create instance", err);

#ifdef USE_VOLK_LOADER
    volkLoadInstance(instance);
#endif /* USE_VOLK_LOADER */

    // ------------- //
    // -- Surface -- //
    // ------------- //
    
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = VK_NULL_HANDLE,
        .hwnd = (HWND) window->GetNativeHandle(),
    };

    PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    vkCreateWin32SurfaceKHR(instance, &win32SurfaceCreateInfo, nullptr, &surface);
}

VulkanContext::~VulkanContext()
{
    vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
    vkDestroyInstance(instance, VK_NULL_HANDLE);
}