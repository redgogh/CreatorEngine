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

#include "Window.h"

#ifdef _WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
#endif /* _WIN32 */
#include <GLFW//glfw3native.h>

Window::Window(uint32_t w, uint32_t h, const char *title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    hwnd = glfwCreateWindow(w, h, title, nullptr, nullptr);
}

Window::~Window()
{
    glfwDestroyWindow(hwnd);
}

void *Window::GetNativeWindow()
{
    return glfwGetWin32Window(hwnd);
}

bool Window::IsShouldClose()
{
    return glfwWindowShouldClose(hwnd);
}

void Window::PollEvents()
{
    glfwPollEvents();
}