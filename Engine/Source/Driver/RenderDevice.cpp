/* -------------------------------------------------------------------------------- *\
|*                                                                                  *|
|*    Copyright (C) 2019-2024 RedGogh All rights reserved.                          *|
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

/* Create by Red Gogh on 2025/4/22 */

#ifdef USE_VOLK_LOADER
#  define VOLK_IMPLEMENTATION
#endif /* USE_VOLK_LOADER */

#define VMA_IMPLEMENTATION

#include "RenderDevice.h"

RenderDevice::RenderDevice(Window *vWindow) : window(vWindow)
{
    VkResult err;

#ifdef USE_VOLK_LOADER
    err = volkInitialize();
    assert(!err && "volkInitialize() failed!");
#endif /* USE_VOLK_LOADER */
    
    _InitVkInstance();
}

RenderDevice::~RenderDevice()
{
    vkDestroyInstance(instance, VK_NULL_HANDLE);
}

void RenderDevice::_InitVkInstance()
{
    VkResult err;
    
    vkEnumerateInstanceVersion(&apiVersion);

    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vernak Engine",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Vernak Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = this->apiVersion
    };

    uint32_t count;
    std::vector<const char *> extensions;

    std::vector<const char *> layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = (uint32_t) std::size(layers),
        .ppEnabledLayerNames = std::data(layers),
        .enabledExtensionCount = (uint32_t) std::size(extensions),
        .ppEnabledExtensionNames = std::data(extensions),
    };

    err = vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &instance);
    assert(!err && "vkCreateInstance() failed!");

#ifdef USE_VOLK_LOADER
    volkLoadInstance(instance);
#endif
}

void RenderDevice::_InitVkSurfaceKHR()
{
    
}