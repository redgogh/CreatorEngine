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

#define VK_ERROR_CHECK(err, msg) GOGH_ASSERT(err == VK_SUCCESS && msg)

namespace VulkanUtils
{
    VkPhysicalDevice PickDiscreteDevice(const std::vector<VkPhysicalDevice> &devices)
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

    void FindQueueIndex(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t *p_index)
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

        GOGH_ERROR("Can't not found queue to support present");
    }

    VkResult PickSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceFormatKHR* pFormat)
    {
        VkResult err;

        uint32_t count;
        err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, VK_NULL_HANDLE);
        if (err)
            return err;

        std::vector<VkSurfaceFormatKHR> formats(count);
        err = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &count, std::data(formats));
        if (err)
            return err;

        for (const auto &item: formats) {
            switch (item.format) {
                case VK_FORMAT_R8G8B8A8_SRGB:
                case VK_FORMAT_B8G8R8A8_SRGB:
                case VK_FORMAT_R8G8B8A8_UNORM:
                case VK_FORMAT_B8G8R8A8_UNORM: {
                    *pFormat = item;
                    goto TAG_PICK_SUITABLE_SURFACE_FORMAT_END;
                }
            }
        }

        assert(count >= 1);
        *pFormat = formats[0];

        GOGH_LOGGER_WARN("[Vulkan] Can't not found suitable surface format, default use first\n");

TAG_PICK_SUITABLE_SURFACE_FORMAT_END:
        return VK_SUCCESS;
    }
    
}