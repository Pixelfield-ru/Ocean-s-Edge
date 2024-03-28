 cmake_minimum_required(VERSION 3.5)

 project(OceansEdgeBuild)

 add_definitions(-DSTB_IMAGE_IMPLEMENTATION)
 add_definitions(-DNOMINMAX)

 IF(NOT MSVC)
     add_definitions(-DSTB_IMAGE_WRITE_IMPLEMENTATION)
 ENDIF()

 add_definitions(-DENTT_USE_ATOMIC)
 add_definitions(-DBOOST_STACKTRACE_USE_ADDR2LINE)
 add_definitions(-DBOOST_STACKTRACE_USE_BACKTRACE)
 add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

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

 #set(SUNGEAR_DIR /home/ilya/pixelfield/SungearEngine)
 #set(SUNGEAR_DIR "D:/Pixelfield/NativeSungearEngine/NativeSungearEngine")
 set(SUNGEAR_DIR "F:/Pixelfield/SungearEngine")

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
 set(SG_RESOURCES_COPY_UI ON)
 set(SG_RESOURCES_COPY_FONTS ON)

 add_subdirectory(${SUNGEAR_DIR}/Externals Externals)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGUtils Sources/SGUtils)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGConsole Sources/SGConsole)
 add_subdirectory(${SUNGEAR_DIR}/Sources/SGCore Sources/SGCore)
 add_subdirectory(${SUNGEAR_DIR}/syslibs syslibs)
 # add_subdirectory(${SUNGEAR_DIR} Sungear)

 add_executable(${PROJECT_NAME} ${OCEANS_EDGE_SOURCES})

 target_link_libraries(${PROJECT_NAME} PUBLIC
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

 if (${SG_RESOURCES_COPY_UI})
     add_custom_target(SGUICopy ALL
             COMMAND ${CMAKE_COMMAND} -E copy_directory
             ${SUNGEAR_DIR}/Resources/SGResources/ui
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
             ${SUNGEAR_DIR}/Resources/SGResources/fonts
             ${CMAKE_BINARY_DIR}/SGResources/fonts
             COMMENT "Copying SGFonts into binary directory")

     add_dependencies(${PROJECT_NAME} SGFontsCopy)

     if(EXISTS ${CMAKE_SOURCE_DIR}/Resources/OEResources/fonts)
         add_custom_target(OEFontsCopy ALL
                 COMMAND ${CMAKE_COMMAND} -E copy_directory
                 ${CMAKE_SOURCE_DIR}/Resources/OEResources/fonts
                 ${CMAKE_BINARY_DIR}/OEResources/fonts
                 COMMENT "Copying SGFonts into binary directory")

         add_dependencies(${PROJECT_NAME} OEFontsCopy)
     endif ()
 endif ()