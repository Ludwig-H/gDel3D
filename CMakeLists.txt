cmake_minimum_required(VERSION 3.18)

# Eviter l'ancien FindCUDA (supprimé) sur CMake récents
if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW)
endif()

project(gDel3D LANGUAGES CXX CUDA)

# Standards
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Toolkit (Thrust inclus)
find_package(CUDAToolkit REQUIRED)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
file(GLOB_RECURSE GDEL3D_CORE
  CONFIGURE_DEPENDS
  GDelFlipping/src/gDel3D/*.cpp
  GDelFlipping/src/gDel3D/*.cu
)
# Exclure éventuels fichiers Visual Studio
list(FILTER GDEL3D_CORE EXCLUDE REGEX ".*\\.(vcxproj|sln)(\\.filters)?$")

# ---------------------------------------------------------------------------
# Détection / choix d'architecture CUDA + fallbacks PTX
# ---------------------------------------------------------------------------
# Essaie de détecter la compute capability via nvidia-smi
execute_process(
  COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader
  OUTPUT_VARIABLE GPU_CAP
  RESULT_VARIABLE GPU_CAP_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(GPU_CAP_RESULT EQUAL 0 AND GPU_CAP)
  string(REPLACE "\n" ";" GPU_CAP_LIST "${GPU_CAP}")
  list(GET GPU_CAP_LIST 0 GPU_CAP_FIRST)
  string(REPLACE "." "" CUDA_ARCH "${GPU_CAP_FIRST}")
else()
  # Valeur raisonnable par défaut (T4/Turing)
  set(CUDA_ARCH 75)
endif()

# Limite supérieure selon la version du toolkit
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.8)
  set(CUDA_MAX_ARCH 86)
elseif(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.0)
  set(CUDA_MAX_ARCH 89)
else()
  set(CUDA_MAX_ARCH 90)
endif()
if(CUDA_ARCH GREATER CUDA_MAX_ARCH)
  set(CUDA_ARCH ${CUDA_MAX_ARCH})
endif()

# Liste finale d'architectures: code natif + PTX de secours
set(CUDA_ARCHITECTURES "${CUDA_ARCH}-real;${CUDA_ARCH}-virtual")
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.0)
  list(PREPEND CUDA_ARCHITECTURES 37-virtual)
endif()
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.0)
  list(PREPEND CUDA_ARCHITECTURES 60-virtual)
endif()
set(CMAKE_CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}" CACHE STRING "" FORCE)
message(STATUS "CUDA architectures: ${CUDA_ARCHITECTURES}")

# ---------------------------------------------------------------------------
# Bibliothèque objet (évite la perte d'objets au link)
# ---------------------------------------------------------------------------
add_library(gdel3d_core OBJECT ${GDEL3D_CORE})
target_include_directories(gdel3d_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CUDAToolkit_INCLUDE_DIRS}
)
set_target_properties(gdel3d_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
)

# Unifier Thrust côté hôte et device
# - Host:   C++ (compilé par g++)
# - Device: CUDA (compilé par nvcc)
target_compile_definitions(gdel3d_core PUBLIC
  THRUST_IGNORE_CUB_VERSION_CHECK=1
  THRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP
  $<$<COMPILE_LANGUAGE:CXX>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP>
  $<$<COMPILE_LANGUAGE:CUDA>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA>
)

# Quelques options de compilation utiles
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(gdel3d_core PRIVATE -Wno-deprecated-declarations)
endif()
if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  target_compile_options(gdel3d_core PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>
  )
endif()

# ---------------------------------------------------------------------------
# Exécutables
# ---------------------------------------------------------------------------
add_executable(gflip3d
  GDelFlipping/src/Demo.cpp
  GDelFlipping/src/DelaunayChecker.cpp
  GDelFlipping/src/InputCreator.cpp
  GDelFlipping/src/RandGen.cpp
  $<TARGET_OBJECTS:gdel3d_core>
)
target_include_directories(gflip3d PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CUDAToolkit_INCLUDE_DIRS}
)
target_link_libraries(gflip3d PRIVATE CUDA::cudart)
set_target_properties(gflip3d PROPERTIES
  CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
  CUDA_SEPARABLE_COMPILATION ON
  LINKER_LANGUAGE CUDA
)
target_compile_definitions(gflip3d PRIVATE
  THRUST_IGNORE_CUB_VERSION_CHECK=1
  THRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP
  $<$<COMPILE_LANGUAGE:CXX>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP>
  $<$<COMPILE_LANGUAGE:CUDA>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA>
)

add_executable(EdgesDelaunay3D
  GDelFlipping/src/EdgesDelaunay3D.cpp
  $<TARGET_OBJECTS:gdel3d_core>
)
target_include_directories(EdgesDelaunay3D PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CUDAToolkit_INCLUDE_DIRS}
)
target_link_libraries(EdgesDelaunay3D PRIVATE CUDA::cudart)
set_target_properties(EdgesDelaunay3D PROPERTIES
  CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
  CUDA_SEPARABLE_COMPILATION ON
  LINKER_LANGUAGE CUDA
)
target_compile_definitions(EdgesDelaunay3D PRIVATE
  THRUST_IGNORE_CUB_VERSION_CHECK=1
  THRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP
  $<$<COMPILE_LANGUAGE:CXX>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP>
  $<$<COMPILE_LANGUAGE:CUDA>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA>
)
