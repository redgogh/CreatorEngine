cmake_minimum_required(VERSION 3.26)
project(vronak)

set(CMAKE_CXX_STANDARD 23)

set(LIBRARY_DIRECTORIES ${PROJECT_SOURCE_DIR}/lib)

include_directories(SYSTEM include ${LIBRARY_DIRECTORIES})

include_directories(
  ${PROJECT_SOURCE_DIR}
)

file(GLOB ENGINE_MODULE_SOURCES
  ${PROJECT_SOURCE_DIR}/*.cpp
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

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/shaders/spir-v
    ${CMAKE_BINARY_DIR}/shaders/spir-v
)

target_link_libraries(${PROJECT_NAME}
  glfw3
)