SET(DEMO_MODULE_NAME "Demo")

FILE(GLOB_RECURSE DEMO_SOURCE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

ADD_EXECUTABLE(${DEMO_MODULE_NAME} ${DEMO_SOURCE_DIRECTORIES})

TARGET_LINK_LIBRARIES(${DEMO_MODULE_NAME} PRIVATE Engine)