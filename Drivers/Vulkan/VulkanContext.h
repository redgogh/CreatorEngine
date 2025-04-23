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
#pragma once

#include <Vdx/Typedef.h>

#ifdef USE_VOLK_LOADER
#  include <volk/volk.h>
#else
#  include <vulkan/vulkan.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <vma/vk_mem_alloc.h>

#include "VulkanUtils.h"
#include "Window/Window.h"

#define VK_CHECK_FATAL_ERROR(msg, err)      \
    do {                                    \
        if (err != VK_SUCCESS)              \
            ERROR_FATAL(msg);               \
    } while(0)

class VulkanContext {
public:
    VulkanContext(const Window* _window);
   ~VulkanContext();
   
   VkInstance GetInstance() const { return instance; }
   VkSurfaceKHR GetSurfaceKHR() const { return surface; }
   
private:
    const Window* window = VK_NULL_HANDLE;
    uint32_t apiVersion = 0;
    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
};