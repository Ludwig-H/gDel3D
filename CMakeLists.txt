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

# Sources principales de la bibliothèque gDel3D
file(GLOB_RECURSE GDEL3D_CORE
  CONFIGURE_DEPENDS
  GDelFlipping/src/gDel3D/*.cpp
  GDelFlipping/src/gDel3D/*.cu
)

# Exclure fichiers Visual Studio
list(FILTER GDEL3D_CORE EXCLUDE REGEX ".*\\.(vcxproj|sln)(\\.filters)?$")

# Executable de démonstration original
add_executable(gflip3d
  ${GDEL3D_CORE}
  GDelFlipping/src/Demo.cpp
  GDelFlipping/src/DelaunayChecker.cpp
  GDelFlipping/src/InputCreator.cpp
  GDelFlipping/src/RandGen.cpp
)

# Nouveau programme pour extraire les arêtes du triangulation
add_executable(EdgesDelaunay3D
  ${GDEL3D_CORE}
  GDelFlipping/src/EdgesDelaunay3D.cpp
)

foreach(target IN ITEMS gflip3d EdgesDelaunay3D)
  target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  )

  # Compiler avec NVCC et lier le runtime CUDA
  target_link_libraries(${target} PRIVATE CUDA::cudart)

  # Détecter et cibler automatiquement l'arch du GPU
  set_target_properties(${target} PROPERTIES
    CUDA_ARCHITECTURES native
    CUDA_SEPARABLE_COMPILATION ON
    POSITION_INDEPENDENT_CODE ON
  )

  # Option utile si Thrust/CUB se plaint de versions
  target_compile_definitions(${target} PRIVATE THRUST_IGNORE_CUB_VERSION_CHECK=1)

  # Eviter que CMake impose -Werror=deprecated-declarations via toolchains exotiques
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
  endif()
  if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
    target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>)
  endif()
endforeach()
