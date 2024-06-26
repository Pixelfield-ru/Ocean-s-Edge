 cmake_minimum_required(VERSION 3.5)

 project(OceansEdgeBuild)

 add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
 add_definitions(-DNOMINMAX)

 add_definitions(-DENTT_USE_ATOMIC)
 add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)
 add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)
 add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

 get_filename_component(VCPKG_CMAKE_ROOT ${CMAKE_TOOLCHAIN_FILE} DIRECTORY)
 get_filename_component(VCPKG_CMAKE_ROOT ${VCPKG_CMAKE_ROOT} DIRECTORY)
 get_filename_component(VCPKG_CMAKE_ROOT ${VCPKG_CMAKE_ROOT} DIRECTORY)

 set(VCPKG_ROOT ${VCPKG_CMAKE_ROOT})

 #list(APPEND CMAKE_PREFIX_PATH ${VCPKG_ROOT}/installed/x64-linux/share/openal-soft)
 list(APPEND CMAKE_PREFIX_PATH ${VCPKG_ROOT}/installed/x64-windows/share/openal-soft)

 set(CMAKE_CXX_STANDARD 23)

 set(CMAKE_VERBOSE_MAKEFILE ON)

 IF(NOT MSVC)
     add_compile_options(-fPIC)
     set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-implicit-fallthrough -Wno-ignored-qualifiers -g -rdynamic -fno-pie -no-pie")
     set(CMAKE_CXX_FLAGS_DEBUG "-O1")
     set(CMAKE_CXX_FLAGS_RELEASE "-O3")
 else()
     add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
     add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

     set(CompilerFlags
             CMAKE_CXX_FLAGS
             CMAKE_CXX_FLAGS_DEBUG
             CMAKE_CXX_FLAGS_RELEASE
             CMAKE_C_FLAGS
             CMAKE_C_FLAGS_DEBUG
             CMAKE_C_FLAGS_RELEASE
     )
     foreach(CompilerFlag ${CompilerFlags})
         string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
     endforeach()

     set(MSVC_RUNTIME "dynamic")
     set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
     set(BUILD_SHARED_LIBS FALSE)
 endif ()

 set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-allow-multiple-definition -Wno-return-type")

 file(GLOB_RECURSE OCEANS_EDGE_SOURCES ../Sources/*.cpp ../Sources/*.h)

 include_directories(${PROJECT_NAME} PUBLIC
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/GLM
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/entt/src
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/imgui
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/Assimp/include
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/OpenGL/include
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/GLFW/include
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/stb
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/bullet3/src
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/spdlog/include
         ${Vulkan_INCLUDE_DIR}
         #${BULLET_INCLUDE_DIR}
         $ENV{SUNGEAR_SOURCES_ROOT}/Sources
         $ENV{SUNGEAR_SOURCES_ROOT}/Externals/fmt/include
 )

 set(SG_RESOURCES_COPY_MODELS ON)
 set(SG_RESOURCES_COPY_SHADERS ON)
 set(SG_RESOURCES_COPY_TEXTURES ON)
 set(SG_RESOURCES_COPY_UI ON)
 set(SG_RESOURCES_COPY_FONTS ON)
 set(SG_RESOURCES_COPY_AUDIO ON)

 add_subdirectory($ENV{SUNGEAR_SOURCES_ROOT}/Sources/SGUtils Sources/SGUtils)
 add_subdirectory($ENV{SUNGEAR_SOURCES_ROOT}/Sources/SGConsole Sources/SGConsole)
 add_subdirectory($ENV{SUNGEAR_SOURCES_ROOT}/Sources/SGCore Sources/SGCore)
 # add_subdirectory(${SUNGEAR_DIR} Sungear)

 add_executable(${PROJECT_NAME} ${OCEANS_EDGE_SOURCES})

 target_link_libraries(${PROJECT_NAME} PUBLIC
         SGCore
 )

 if (${SG_RESOURCES_COPY_MODELS})
     add_custom_target(SGModelsCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             $ENV{SUNGEAR_SOURCES_ROOT}/Resources/SGResources/models
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
             $ENV{SUNGEAR_SOURCES_ROOT}/Resources/SGResources/shaders
             ${CMAKE_BINARY_DIR}/SGResources/shaders
             COMMENT "Copying SGShaders into binary directory")

     add_dependencies(${PROJECT_NAME} SGShadersCopy)

     if(EXISTS ${CMAKE_SOURCE_DIR}/Resources/OEResources/shaders)
         add_custom_target(OEShadersCopy ALL
                 COMMAND ${CMAKE_COMMAND} -E copy_directory
                 ${CMAKE_SOURCE_DIR}/Resources/OEResources/shaders
                 ${CMAKE_BINARY_DIR}/OEResources/shaders
                 COMMENT "Copying OEShaders into binary directory")

         add_dependencies(${PROJECT_NAME} OEShadersCopy)
     endif()
 endif ()

 if (${SG_RESOURCES_COPY_TEXTURES})
     add_custom_target(SGTexturesCopy ALL
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          $ENV{SUNGEAR_SOURCES_ROOT}/Resources/SGResources/textures
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

 if (${SG_RESOURCES_COPY_UI})
     add_custom_target(SGUICopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             $ENV{SUNGEAR_SOURCES_ROOT}/Resources/SGResources/ui
             ${CMAKE_BINARY_DIR}/SGResources/ui
             COMMENT "Copying SGUI into binary directory")

     add_dependencies(${PROJECT_NAME} SGUICopy)

     if(EXISTS ${CMAKE_SOURCE_DIR}/Resources/OEResources/ui)
         add_custom_target(OEUICopy ALL
                 COMMAND ${CMAKE_COMMAND} -E copy_directory
                 ${CMAKE_SOURCE_DIR}/Resources/OEResources/ui
                 ${CMAKE_BINARY_DIR}/OEResources/ui
                 COMMENT "Copying OEUI into binary directory")

         add_dependencies(${PROJECT_NAME} OEUICopy)
     endif()
 endif ()

 if (${SG_RESOURCES_COPY_FONTS})
     add_custom_target(SGFontsCopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             $ENV{SUNGEAR_SOURCES_ROOT}/Resources/SGResources/fonts
             ${CMAKE_BINARY_DIR}/SGResources/fonts
             COMMENT "Copying SGFonts into binary directory")

     add_dependencies(${PROJECT_NAME} SGFontsCopy)

     if(EXISTS ${CMAKE_SOURCE_DIR}/Resources/OEResources/fonts)
         add_custom_target(OEFontsCopy ALL
                 COMMAND ${CMAKE_COMMAND} -E copy_directory
                 ${CMAKE_SOURCE_DIR}/Resources/OEResources/fonts
                 ${CMAKE_BINARY_DIR}/OEResources/fonts
                 COMMENT "Copying OEFonts into binary directory")

         add_dependencies(${PROJECT_NAME} OEFontsCopy)
     endif ()
 endif ()

 if (${SG_RESOURCES_COPY_AUDIO})
     if(EXISTS ${CMAKE_SOURCE_DIR}/Resources/OEResources/audio)
         add_custom_target(OEAudioCopy ALL
                 COMMAND ${CMAKE_COMMAND} -E copy_directory
                 ${CMAKE_SOURCE_DIR}/Resources/OEResources/audio
                 ${CMAKE_BINARY_DIR}/OEResources/audio
                 COMMENT "Copying OEAudio into binary directory")

         add_dependencies(${PROJECT_NAME} OEAudioCopy)
     endif ()
 endif ()