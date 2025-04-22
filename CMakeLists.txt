cmake_minimum_required(VERSION 3.26)
project(vrc)

set(CMAKE_CXX_STANDARD 23)

set(LIBRARY_DIRECTORIES ${PROJECT_SOURCE_DIR}/Lib)

add_definitions(-DUSE_VOLK_LOADER)

include_directories(SYSTEM include ${LIBRARY_DIRECTORIES} ${LIBRARY_DIRECTORIES}/imgui)

include_directories(
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/Modules
)

file(GLOB ENGINE_MODULE_SOURCES
  ${PROJECT_SOURCE_DIR}/*.cpp
  ${PROJECT_SOURCE_DIR}/Driver/*.cpp
  ${PROJECT_SOURCE_DIR}/Driver/Vulkan/*.cpp
  # -- Modules --
  ${PROJECT_SOURCE_DIR}/Modules/Window/*.cpp
  # -- ImGui --
  ${LIBRARY_DIRECTORIES}/imgui/*.cpp
  ${LIBRARY_DIRECTORIES}/imgui/backends/imgui_impl_glfw.cpp
  ${LIBRARY_DIRECTORIES}/imgui/backends/imgui_impl_vulkan.cpp
  ${LIBRARY_DIRECTORIES}/ImGuizmo/ImGuizmo.cpp
  ${LIBRARY_DIRECTORIES}/imnodes/imnodes.cpp
)

set(GLFW_LIBRARY_DIRECTORY ${LIBRARY_DIRECTORIES}/GLFW/lib-mingw-w64)

if (MSVC)
  set(GLFW_LIBRARY_DIRECTORY ${LIBRARY_DIRECTORIES}/GLFW/lib-vc2022)
endif()

link_directories(
  ${LIBRARY_DIRECTORIES}/vulkan/lib
  ${GLFW_LIBRARY_DIRECTORY}
)

add_executable(${PROJECT_NAME} ${ENGINE_MODULE_SOURCES})

target_link_libraries(${PROJECT_NAME}
  glfw3
)