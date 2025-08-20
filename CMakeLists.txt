cmake_minimum_required(VERSION 3.18)

# Eviter FindCUDA obsolète
if(POLICY CMP0146)
  cmake_policy(SET CMP0146 NEW)
endif()

project(gDel3D LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

find_package(CUDAToolkit REQUIRED)

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------
file(GLOB_RECURSE GDEL3D_CORE
  CONFIGURE_DEPENDS
  GDelFlipping/src/gDel3D/*.cpp
  GDelFlipping/src/gDel3D/*.cu
)
list(FILTER GDEL3D_CORE EXCLUDE REGEX ".*\\.(vcxproj|sln)(\\.filters)?$")

# Ces CPP définissent des fonctions appelées depuis des .cu avec thrust::host_vector
# On les compile aussi via NVCC pour aligner l’ABI Thrust côté device.
set(CUDA_HOST_BRIDGE_CPP
  GDelFlipping/src/gDel3D/CPU/PredWrapper.cpp
  GDelFlipping/src/gDel3D/CPU/Splaying.cpp
)
foreach(f ${CUDA_HOST_BRIDGE_CPP})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${f})
    set_source_files_properties(${f} PROPERTIES LANGUAGE CUDA)
  endif()
endforeach()

# ------------------------------------------------------------------------------
# Détection d’arch + PTX fallback
# ------------------------------------------------------------------------------
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
  set(CUDA_ARCH 75) # défaut raisonnable (T4)
endif()

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

set(CUDA_ARCHITECTURES "${CUDA_ARCH}-real;${CUDA_ARCH}-virtual")
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.0)
  list(PREPEND CUDA_ARCHITECTURES 37-virtual)
endif()
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.0)
  list(PREPEND CUDA_ARCHITECTURES 52-virtual)
endif()
message(STATUS "CUDA architectures: ${CUDA_ARCHITECTURES}")

# ------------------------------------------------------------------------------
# Lib objet
# ------------------------------------------------------------------------------
add_library(gdel3d_core OBJECT ${GDEL3D_CORE})
target_include_directories(gdel3d_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CUDAToolkit_INCLUDE_DIRS}
)
set_target_properties(gdel3d_core PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
  CUDA_SEPARABLE_COMPILATION ON
)

# Unifier Thrust: host=CPP, device=CUDA PARTOUT (CXX et CUDA)
target_compile_definitions(gdel3d_core PUBLIC
  THRUST_IGNORE_CUB_VERSION_CHECK=1
  THRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP
  THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(gdel3d_core PRIVATE -Wno-deprecated-declarations)
endif()
if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  target_compile_options(gdel3d_core PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>
  )
endif()

# ------------------------------------------------------------------------------
# Exécutables (on injecte directement les objets de la lib)
# ------------------------------------------------------------------------------
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
  THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA
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
  THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA
)
