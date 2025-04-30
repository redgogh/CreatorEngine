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

// std
#include <Typedef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    
GOGH_API void Gogh_Engine_Init(uint32_t w, uint32_t h, const char *title);
GOGH_API void Gogh_Engine_Terminate();

GOGH_API GOGH_BOOL Gogh_Engine_IsShouldClose();
GOGH_API void Gogh_Engine_PollEvents();

GOGH_API void Gogh_Engine_BeginNewFrame();
GOGH_API void Gogh_Engine_EndNewFrame();

#ifdef __cplusplus
}
#endif