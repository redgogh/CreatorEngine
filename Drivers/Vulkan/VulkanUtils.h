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

#pragma once

#include <Vdx/Error.h>

namespace VulkanUtils {

    static VkPhysicalDevice PickDiscreteDevice(const std::vector<VkPhysicalDevice> &devices)
    {
        for (const auto &device: devices) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);

            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                return device;
        }

        assert(std::size(devices) > 0);
        return devices[0];
    }

    static void FindQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
    {
        uint32_t count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, VK_NULL_HANDLE);

        std::vector<VkQueueFamilyProperties> properties(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, std::data(properties));

        for (uint32_t i = 0; i < count; i++) {
            VkQueueFamilyProperties property = properties[i];
            VkBool32 supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
            if ((property.queueFlags & VK_QUEUE_GRAPHICS_BIT) && supported) {
                *p_index = i;
                return;
            }
        }

        ERROR_FATAL("Can't not found queue to support present");
    }

}