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

#include "CommandList.h"

CommandList::CommandList(VkDevice _device, VkCommandPool _commandPool, VkQueue _queue)
    : device(_device), commandPool(_commandPool), queue(_queue) 
{
    VkResult err;

    GOGH_LOGGER_DEBUG("[Vulkan] Creating command list object, (CommandList: %p)", this);
    
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    err = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

    if (err != VK_SUCCESS) {
        GOGH_LOGGER_WARN("[Vulkan] Failed allocating VkCommandBuffer (VkCommandPool: %p)", commandPool);
        return;
    }

    GOGH_LOGGER_DEBUG("[Vulkan] Allocated VkCommandBuffer: %p (VkCommandPool: %p)", commandBuffer, commandPool);
}

CommandList::~CommandList() 
{
    GOGH_LOGGER_DEBUG("[Vulkan] Destroying command list object, (CommandList: %p)", this);
    GOGH_LOGGER_DEBUG("[Vulkan] Free VkCommandBuffer: %p (VkCommandPool: %p)", commandBuffer, commandPool);
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}