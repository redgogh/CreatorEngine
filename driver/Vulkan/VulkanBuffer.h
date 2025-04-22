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
#pragma once

#include "Driver/VdxBuffer.h"

#include "VulkanInclude.h"

class VulkanBuffer : public VdxBuffer {
public:
    VulkanBuffer(Driver* _driver, VkBuffer _buf, VmaAllocation _alloc, VmaAllocationInfo &_alloci, size_t _size)
        : driver(_driver), vkBuffer(_buf), allocation(_alloc), allocationInfo(_alloci), size(_size) {}
        
    virtual ~VulkanBuffer() override 
    {
        if (vkBuffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(driver->allocator, vkBuffer, allocation);
    }
    
    virtual void *GetHandle() override { return this; }
    virtual size_t GetSize() override { return size; }

    virtual void WriteMemory(size_t offset, size_t size, const void* data) override;
    virtual void ReadMemory(size_t offset, size_t size, const void* data) override;
    
private:
    Driver* driver = VK_NULL_HANDLE;
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
    VkDeviceSize size = 0;
};
