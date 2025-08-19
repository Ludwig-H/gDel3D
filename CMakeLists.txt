cmake_minimum_required(VERSION 3.24)

# Evite l'ancien module FindCUDA (supprimé) si un CMake trop récent est utilisé
if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW)
endif()

project(gDel3D LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Toolkit officiel (headers Thrust inclus)
find_package(CUDAToolkit REQUIRED)

# Inclure les sources .cpp/.cu du dossier src
file(GLOB_RECURSE GDEL3D_SOURCES
  CONFIGURE_DEPENDS
  GDelFlipping/src/*.cpp
  GDelFlipping/src/*.cu
)

# Exclure fichiers Visual Studio
list(FILTER GDEL3D_SOURCES EXCLUDE REGEX ".*\\.(vcxproj|sln)(\\.filters)?$")

add_executable(gflip3d ${GDEL3D_SOURCES})

target_include_directories(gflip3d PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}
)

# Compiler avec NVCC et lier le runtime CUDA
target_link_libraries(gflip3d PRIVATE CUDA::cudart)

# Détecter et cibler automatiquement l'arch du GPU
set_target_properties(gflip3d PROPERTIES
  CUDA_ARCHITECTURES native
  CUDA_SEPARABLE_COMPILATION ON
  POSITION_INDEPENDENT_CODE ON
)

# Option utile si Thrust/CUB se plaint de versions
target_compile_definitions(gflip3d PRIVATE THRUST_IGNORE_CUB_VERSION_CHECK=1)

# Eviter que CMake impose -Werror=deprecated-declarations via toolchains exotiques
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(gflip3d PRIVATE -Wno-deprecated-declarations)
endif()
if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  target_compile_options(gflip3d PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>)
endif()
