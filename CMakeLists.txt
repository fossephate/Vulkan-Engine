cmake_minimum_required(VERSION 3.16)
project(VulkanEngine CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(ENGINE_DIR ${CMAKE_SOURCE_DIR}/vulkan)

# Homebrew prefix (arm64 default)
execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT HOMEBREW_PREFIX)
  set(HOMEBREW_PREFIX /opt/homebrew)
endif()

find_library(SDL2_LIB SDL2 PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
find_library(ASSIMP_LIB assimp PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
find_library(VULKAN_LIB vulkan PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
find_library(BULLET_DYNAMICS_LIB BulletDynamics PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
find_library(BULLET_COLLISION_LIB BulletCollision PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
find_library(BULLET_LINEARMATH_LIB LinearMath PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)

add_executable(vulkanEngine
  ${ENGINE_DIR}/src/main.cpp
  ${ENGINE_DIR}/src/vulkanClasses/camera.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanAndroid.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanApp.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanContext.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanDebug.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanFrameBuffer.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanMesh.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanMeshLoader.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanModel.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanSkinnedMesh.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanswapchain.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanTextOverlay.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkanTextureLoader.cpp
  ${ENGINE_DIR}/src/vulkanClasses/vulkantools.cpp
  ${ENGINE_DIR}/src/bulletClasses/PhysicsManager.cpp
  ${ENGINE_DIR}/src/bulletClasses/PhysicsObject.cpp
  ${ENGINE_DIR}/include/external/imgui/imgui.cpp
  ${ENGINE_DIR}/include/external/imgui/imgui_draw.cpp
  ${ENGINE_DIR}/include/external/imgui/imgui_demo.cpp
)

# The bundled SDL2 headers are ancient (pre-Vulkan) — shim Homebrew's SDL2 in
# front of them via a symlink dir so <SDL2/...> resolves to the modern headers.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/compat_include)
file(CREATE_LINK ${HOMEBREW_PREFIX}/include/SDL2 ${CMAKE_BINARY_DIR}/compat_include/SDL2 SYMBOLIC)
# Same for assimp: the bundled headers have a different aiString ABI than the
# Homebrew libassimp we link (symptom: all texture paths lose their first 4 chars).
file(CREATE_LINK ${HOMEBREW_PREFIX}/include/assimp ${CMAKE_BINARY_DIR}/compat_include/assimp SYMBOLIC)

# Bundled headers first so the project's Vulkan 1.0.46 vulkan.hpp wins over any
# vulkan headers living in the Homebrew include dir.
target_include_directories(vulkanEngine BEFORE PRIVATE
  ${CMAKE_BINARY_DIR}/compat_include
  ${ENGINE_DIR}/include
  ${ENGINE_DIR}/include/main
  ${ENGINE_DIR}/include/base
  ${ENGINE_DIR}/include/vulkanClasses
  ${ENGINE_DIR}/include/external
  ${ENGINE_DIR}/include/external/imgui
)

target_include_directories(vulkanEngine SYSTEM PRIVATE
  ${HOMEBREW_PREFIX}/include
  ${HOMEBREW_PREFIX}/include/bullet
)

# Old codebase: silence the warning avalanche; the second flag downgrades a
# new Clang 21 default-error that old gli trips on ("obj.template fetch(...)").
target_compile_options(vulkanEngine PRIVATE -w
  # New Clang 21 default-error that old gli trips on ("obj.template fetch(...)")
  -Wno-missing-template-arg-list-after-template-kw
  # MSVC-era idiom used all over main.cpp: &container.get("name") on a by-value
  # return, consumed before end of full expression
  -Wno-address-of-temporary
)

target_link_libraries(vulkanEngine PRIVATE
  ${SDL2_LIB}
  ${ASSIMP_LIB}
  ${VULKAN_LIB}
  ${BULLET_DYNAMICS_LIB}
  ${BULLET_COLLISION_LIB}
  ${BULLET_LINEARMATH_LIB}
)

# Make the assets reachable next to the binary
add_custom_command(TARGET vulkanEngine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E create_symlink
          ${ENGINE_DIR}/src/assets
          $<TARGET_FILE_DIR:vulkanEngine>/assets
)
