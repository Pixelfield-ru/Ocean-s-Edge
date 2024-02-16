 cmake_minimum_required(VERSION 3.22)

 project(OceansEdgeBuild)

 add_definitions(-DSTB_IMAGE_IMPLEMENTATION)

 set(CMAKE_CXX_STANDARD 23)

 set(CMAKE_VERBOSE_MAKEFILE ON)

 add_compile_options(-fPIC)

 set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-implicit-fallthrough -Wno-ignored-qualifiers")
 set(CMAKE_CXX_FLAGS_DEBUG "-O0")
 set(CMAKE_CXX_FLAGS_RELEASE "-O3")
 message("src dir: " ${CMAKE_SOURCE_DIR})

 if(MSVC)
     set(MSVC_RUNTIME "dynamic")
     set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
     set(BUILD_SHARED_LIBS TRUE)
 endif()

 set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-allow-multiple-definition -Wno-return-type")

 file(GLOB_RECURSE OCEANS_EDGE_SOURCES ../Sources/*.cpp ../Sources/*.h)

 set(SUNGEAR_DIR /home/ilya/pixelfield/SungearEngine)

 include_directories(${PROJECT_NAME} PUBLIC
         ${SUNGEAR_DIR}/Externals
         ${SUNGEAR_DIR}/Externals/GLM
         ${SUNGEAR_DIR}/Externals/entt/src
         ${SUNGEAR_DIR}/Externals/imgui
         ${SUNGEAR_DIR}/Externals/Assimp/include
         ${SUNGEAR_DIR}/Externals/OpenGL/include
         ${SUNGEAR_DIR}/Externals/GLFW/include
         ${SUNGEAR_DIR}/Externals/stb
         ${SUNGEAR_DIR}/Externals/bullet3/src
         ${SUNGEAR_DIR}/Externals/spdlog/include
         ${Vulkan_INCLUDE_DIR}
         #${BULLET_INCLUDE_DIR}
         ${SUNGEAR_DIR}/Sources
         ${SUNGEAR_DIR}/Externals/fmt/include
 )

 set(SG_RESOURCES_COPY_MODELS OFF)
 set(SG_RESOURCES_COPY_SHADERS ON)
 set(SG_RESOURCES_COPY_TEXTURES OFF)

 add_subdirectory(${SUNGEAR_DIR}/Externals Externals)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGUtils Sources/SGUtils)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGConsole Sources/SGConsole)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGCore Sources/SGCore)
 add_subdirectory(${SUNGEAR_DIR}/syslibs syslibs)
 # add_subdirectory(${SUNGEAR_DIR} Sungear)

 add_executable(${PROJECT_NAME} ${OCEANS_EDGE_SOURCES}
         Systems/DayNightCycleSystem.cpp
         Systems/DayNightCycleSystem.h)

 target_link_libraries(${PROJECT_NAME} PRIVATE
         SGCore
 )

 if (${SG_RESOURCES_COPY_MODELS})
     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/models ${CMAKE_BINARY_DIR}/SGResources/models)
 endif ()

 if (${SG_RESOURCES_COPY_SHADERS})
     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/shaders ${CMAKE_BINARY_DIR}/SGResources/shaders)
 endif ()

 if (${SG_RESOURCES_COPY_TEXTURES})
     add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/textures ${CMAKE_BINARY_DIR}/SGResources/textures)
 endif ()