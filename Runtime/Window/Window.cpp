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
#include "Window.h"

static unsigned int WCounter = 0;

Window::Window(uint32_t w, uint32_t h, const char *title)
    : width(w), height(h)
{
    if (!WCounter) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    hwnd = glfwCreateWindow(w, h, title, nullptr, nullptr);

    if (!hwnd)
        throw std::runtime_error("Failed to create GLFW window");

    WCounter++;
}

Window::~Window()
{
    glfwDestroyWindow(hwnd);
    if (!(WCounter--))
        glfwTerminate();
}

bool Window::IsShouldClose() const
{
    return glfwWindowShouldClose(hwnd);
}

GLFWwindow *Window::GetHandle() const
{
    return hwnd;
}

void *Window::GetNativeHandle() const
{
    return glfwGetWin32Window(hwnd);
}

void Window::PollEvents() const
{
    glfwPollEvents();
}