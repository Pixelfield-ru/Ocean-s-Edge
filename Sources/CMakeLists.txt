 cmake_minimum_required(VERSION 3.22)

 project(OceansEdgeBuild)

 add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
 add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)

 set(CMAKE_CXX_STANDARD 23)

 set(CMAKE_VERBOSE_MAKEFILE ON)

 add_compile_options(-fPIC)

 set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-implicit-fallthrough -Wno-ignored-qualifiers")
 set(CMAKE_CXX_FLAGS_DEBUG "-O1")
 set(CMAKE_CXX_FLAGS_RELEASE "-O3")

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

 set(SG_RESOURCES_COPY_MODELS ON)
 set(SG_RESOURCES_COPY_SHADERS ON)
 set(SG_RESOURCES_COPY_TEXTURES ON)

 add_subdirectory(${SUNGEAR_DIR}/Externals Externals)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGUtils Sources/SGUtils)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGConsole Sources/SGConsole)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGCore Sources/SGCore)
 add_subdirectory(${SUNGEAR_DIR}/syslibs syslibs)
 # add_subdirectory(${SUNGEAR_DIR} Sungear)

 add_executable(${PROJECT_NAME} ${OCEANS_EDGE_SOURCES})

 target_link_libraries(${PROJECT_NAME} PRIVATE
         ${BULLET_COLLISION_LIBRARY}
         ${BULLET_DYNAMICS_LIBRARY}
         ${BULLET_MATH_LIBRARY}
         SGCore
 )

 if (${SG_RESOURCES_COPY_MODELS})
     add_custom_target(SGModelsCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/models
             ${CMAKE_BINARY_DIR}/SGResources/models
             COMMENT "Copying SGModels into binary directory")

     add_dependencies(${PROJECT_NAME} SGModelsCopy)

     add_custom_target(OEModelsCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${CMAKE_SOURCE_DIR}/Resources/OEResources/models
             ${CMAKE_BINARY_DIR}/OEResources/models
             COMMENT "Copying OEModels into binary directory")

     add_dependencies(${PROJECT_NAME} OEModelsCopy)
 endif ()

 if (${SG_RESOURCES_COPY_SHADERS})
     add_custom_target(SGShadersCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/shaders
             ${CMAKE_BINARY_DIR}/SGResources/shaders
             COMMENT "Copying SGShaders into binary directory")

     add_dependencies(${PROJECT_NAME} SGShadersCopy)
 endif ()

 if (${SG_RESOURCES_COPY_TEXTURES})
     add_custom_target(SGTexturesCopy ALL
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${SUNGEAR_DIR}/Resources/SGResources/textures
          ${CMAKE_BINARY_DIR}/SGResources/textures
          COMMENT "Copying SGTextures into binary directory")

     add_dependencies(${PROJECT_NAME} SGTexturesCopy)

     add_custom_target(OETexturesCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${CMAKE_SOURCE_DIR}/Resources/OEResources/textures
             ${CMAKE_BINARY_DIR}/OEResources/textures
             COMMENT "Copying OETextures into binary directory")

     add_dependencies(${PROJECT_NAME} OETexturesCopy)
 endif ()