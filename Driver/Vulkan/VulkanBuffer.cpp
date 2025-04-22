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
#include "VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(const VulkanDriver* _driver, size_t _size, BufferUsageFlags usage)
    : driver(_driver), size(_size)
{
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage == BUFFER_USAGE_VERTEX_BIT ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    vmaCreateBuffer(driver->allocator, &bufferCreateInfo, &allocationCreateInfo, &vkBuffer, &allocation, &allocationInfo);
}

VulkanBuffer::~VulkanBuffer()
{
    vmaDestroyBuffer(driver->allocator, vkBuffer, allocation);
}

void VulkanBuffer::ReadMemory(size_t offset, size_t length, void *data)
{
    void* ptr;
    vmaMapMemory(driver->allocator, allocation, &ptr);
    memcpy(data, ptr, length);
    vmaUnmapMemory(driver->allocator, allocation);
}

void VulkanBuffer::WriteMemory(size_t offset, size_t length, const void *data)
{
    void* ptr;
    vmaMapMemory(driver->allocator, allocation, &ptr);
    memcpy(ptr, data, length);
    vmaUnmapMemory(driver->allocator, allocation);
}