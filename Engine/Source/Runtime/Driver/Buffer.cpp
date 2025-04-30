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

#include "Buffer.h"

Buffer::Buffer(VmaAllocator _allocator, size_t _allocateSize, VkBufferUsageFlags _usage)
    : allocator(_allocator), allocateSize(_allocateSize), usage(_usage)
{
    VkResult err;
    
    GOGH_LOGGER_DEBUG("[Vulkan] Creating Buffer, allocator=%p, size=%zu, usage=%u (Buffer: %p)", allocator, allocateSize, usage, this);
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = allocateSize,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocationCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    err = vmaCreateBuffer(allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo);
    
    if (err != VK_SUCCESS) {
        GOGH_LOGGER_ERROR("[Vulkan] Failed to create buffer, allocator=%p, size=%zu, usage=%u (Buffer: %p)", allocator, allocateSize, usage, this);
        return;
    }

    GOGH_LOGGER_DEBUG("[Vulkan] Create buffer successful, allocator=%p, size=%zu, usage=%u (Buffer: %p)", allocator, allocateSize, usage, this);
}

Buffer::~Buffer()
{
    GOGH_LOGGER_DEBUG("[Vulkan] Destroying buffer, allocator=%p, size=%zu, usage=%u (Buffer: %p)", allocator, allocateSize, usage, this);
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void Buffer::ReadBack(size_t offset, size_t size, void *dst)
{
    void* src;
    vmaMapMemory(allocator, allocation, &src);
    memcpy(dst, (static_cast<char*>(src) + offset), size);
    vmaUnmapMemory(allocator, allocation);
}

void Buffer::Write(size_t offset, size_t size, const void* src)
{
    void* dst;
    vmaMapMemory(allocator, allocation, &dst);
    memcpy((static_cast<char*>(dst) + offset), src, size);
    vmaUnmapMemory(allocator, allocation);
}