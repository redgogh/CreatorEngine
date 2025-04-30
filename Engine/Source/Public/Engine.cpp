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

#include <Engine/Engine.h>

#include "Window/Window.h"
#include "Driver/RenderDevice.h"

// std
#include <memory>

// include
#include <Error.h>
#include <Logger.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"

struct EngineContext
{
    std::unique_ptr<Window> window;
    std::unique_ptr<RenderDevice> renderDevice;
};

static EngineContext* engine = nullptr;

GOGH_API void Gogh_Engine_Init(uint32_t w, uint32_t h, const char *title)
{
    if (engine)
        return;
    
    engine = new EngineContext();
    
    engine->window = std::make_unique<Window>(w, h, title);
    engine->renderDevice = std::make_unique<RenderDevice>(engine->window.get());

    GOGH_LOGGER_DEBUG("[Engine] Initialize successful, engine has start");
}

GOGH_API void Gogh_Engine_Terminate()
{
    GOGH_LOGGER_DEBUG("[Engine] Terminating engine...");

    if (engine) {
        delete engine;
        engine = nullptr;
        GOGH_LOGGER_DEBUG("[Engine] Engine termination successful");
    } else {
        GOGH_LOGGER_WARN("[Engine] Engine was already terminated or not initialized");
    }
}

GOGH_API GOGH_BOOL Gogh_Engine_IsShouldClose()
{
    return engine->window->IsShouldClose();
}

GOGH_API void Gogh_Engine_PollEvents()
{
    engine->window->PollEvents();
}

GOGH_API void Gogh_Engine_BeginNewFrame()
{
    
}

GOGH_API void Gogh_Engine_EndNewFrame()
{
    
}

#pragma clang diagnostic pop