cmake_minimum_required(VERSION 3.15)
project(Toyxx)

list(PREPEND CMAKE_PREFIX_PATH /home/edwde/.local/micromamba/envs/cxx)
list(PREPEND CMAKE_LIBRARY_PATH /home/edwde/.local/micromamba/envs/cxx/lib)
list(PREPEND CMAKE_INCLUDE_PATH /home/edwde/.local/micromamba/envs/cxx/include)
list(PREPEND CMAKE_INCLUDE_PATH ../Extern/)
list(PREPEND CMAKE_INCLUDE_PATH ../imgui-1.90.4/)
list(PREPEND CMAKE_LIBRARY_PATH ${CMAKE_BINARY_DIR})
include(../build/conan_toolchain.cmake)

add_definitions(-DGLFW_INCLUDE_NONE)
find_package(glfw3   REQUIRED)
find_package(glad    REQUIRED)
find_package(glm     REQUIRED)
find_package(Bullet  REQUIRED)
find_package(openfbx REQUIRED)

add_executable(Toyxx
    ../Data/Gui.glsl
    ../Data/Base.frag
    ../Data/Unlit.glsl
    ../Data/Bake2.frag
    ../Data/Bake3.glsl
    Utility.cpp
    Precompiled.cpp
    Main.cpp
Debug.cpp Debug.h)
target_include_directories(Toyxx PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_directories(Toyxx PRIVATE ${CMAKE_LIBRARY_PATH})
target_link_libraries(Toyxx PRIVATE
    glfw glad dl
    BulletSoftBody
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
)

add_library(Toyxx_Plugin SHARED Plugin.cpp AF.cpp AF.h Database.cpp)
target_include_directories(Toyxx_Plugin PRIVATE ${CMAKE_INCLUDE_PATH})
target_link_directories(Toyxx_Plugin PRIVATE ${CMAKE_LIBRARY_PATH})
target_link_libraries(Toyxx_Plugin PRIVATE
    Bullet3Common
    BulletDynamics
    BulletCollision
    LinearMath
    OpenFBX
    miniz
)
