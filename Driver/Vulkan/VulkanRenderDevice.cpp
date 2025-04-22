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
#include "VulkanRenderDevice.h"

VulkanRenderDevice::VulkanRenderDevice()
{

}

VulkanRenderDevice::~VulkanRenderDevice()
{
    vkDestroyDevice(driver->device, VK_NULL_HANDLE);
    vkDestroyInstance(driver->instance, VK_NULL_HANDLE);

    MemoryDelete(driver);
}

Buffer *VulkanRenderDevice::CreateBuffer(size_t size, BufferUsageFlags usage)
{
    return MemoryNew<VulkanBuffer>(driver, size, usage);
}

void VulkanRenderDevice::DestroyBuffer(Buffer *buffer)
{
    return MemoryDelete(driver);
}