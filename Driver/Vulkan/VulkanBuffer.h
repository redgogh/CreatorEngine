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

#include "Driver/Buffer.h"

#include "VulkanInclude.h"

class VulkanBuffer : public Buffer {
public:
    VulkanBuffer(const VulkanDriver* _driver, size_t _size, BufferUsageFlags usage);
    virtual ~VulkanBuffer() override;

    virtual size_t GetSize() override { return size; }
    virtual void ReadMemory(size_t offset, size_t length, void* data) override;
    virtual void WriteMemory(size_t offset, size_t length, const void* data) override;

private:
    const VulkanDriver* driver = VK_NULL_HANDLE;

    uint32_t size = 0;
    VkBuffer vkBuffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};

};