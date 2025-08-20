cmake_minimum_required(VERSION 3.18)

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

# Bibliothèque centrale contenant toutes les implémentations CPU/GPU
#
# Les exécutables finaux sont sujets à des "undefined reference" si l'on
# empaquette ces objets dans une bibliothèque statique classique : le
# linker omet certains fichiers requis car ils ne sont pas directement
# référencés.  Pour éviter d'utiliser des options spécifiques comme
# `--whole-archive`, on construit plutôt une **object library** dont les
# objets seront directement ajoutés aux exécutables.
add_library(gdel3d_core OBJECT ${GDEL3D_CORE})
target_include_directories(gdel3d_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
  ${CUDAToolkit_INCLUDE_DIRS}
)

# Détection et ciblage automatique de l'architecture GPU avec limitation
execute_process(
  COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader
  OUTPUT_VARIABLE GPU_CAP
  RESULT_VARIABLE GPU_CAP_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
# if(GPU_CAP_RESULT EQUAL 0 AND GPU_CAP)
#   string(REPLACE "." "" CUDA_ARCH ${GPU_CAP})
# else()
#   # Valeur par défaut raisonnable (GPU Turing)
set(CUDA_ARCH 75)
# endif()

# nvcc des toolkits anciens n'acceptent pas les architectures les plus récentes.
# On limite donc l'architecture à la plus élevée prise en charge par le toolkit
# et on fournit un PTX de secours pour permettre la recompilation JIT.
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

# Assemblage de la liste finale des architectures.
# Des PTX de secours sont ajoutés pour les GPU plus anciens si le
# toolkit les prend en charge, afin d'éviter l'erreur «invalid device
# function». Le driver pourra les recompiler dynamiquement si besoin.
set(CUDA_ARCH_NATIVE "${CUDA_ARCH}-real;${CUDA_ARCH}-virtual")
set(PTX_FALLBACKS ${CUDA_ARCH})
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 11.0)
  list(APPEND PTX_FALLBACKS 37-virtual)
endif()
if(CMAKE_CUDA_COMPILER_VERSION VERSION_LESS 12.0)
  list(APPEND PTX_FALLBACKS 52-virtual)
endif()
set(CUDA_ARCHITECTURES ${PTX_FALLBACKS})
list(APPEND CUDA_ARCHITECTURES ${CUDA_ARCH_NATIVE})

set_target_properties(gdel3d_core PROPERTIES
  CUDA_ARCHITECTURES "${CUDA_ARCHITECTURES}"
  CUDA_SEPARABLE_COMPILATION ON
  POSITION_INDEPENDENT_CODE ON
)

# Option utile si Thrust/CUB se plaint de versions et forcer le backend C++
# pour les fichiers compilés avec le compilateur hôte afin d'éviter
# l'inclusion de CUB qui provoque des erreurs lorsqu'il n'est pas
# compilé par NVCC.
target_compile_definitions(gdel3d_core PUBLIC
  THRUST_IGNORE_CUB_VERSION_CHECK=1
  $<$<COMPILE_LANGUAGE:CXX>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP>
)

# Eviter que CMake impose -Werror=deprecated-declarations via toolchains exotiques
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(gdel3d_core PRIVATE -Wno-deprecated-declarations)
endif()
if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
  target_compile_options(gdel3d_core PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>)
endif()

# Executable de démonstration original
add_executable(gflip3d
  GDelFlipping/src/Demo.cpp
  GDelFlipping/src/DelaunayChecker.cpp
  GDelFlipping/src/InputCreator.cpp
  GDelFlipping/src/RandGen.cpp
  $<TARGET_OBJECTS:gdel3d_core>
)
target_link_libraries(gflip3d PRIVATE gdel3d_core CUDA::cudart)

# Nouveau programme pour extraire les arêtes de la triangulation
add_executable(EdgesDelaunay3D
  GDelFlipping/src/EdgesDelaunay3D.cpp
  $<TARGET_OBJECTS:gdel3d_core>
)
target_link_libraries(EdgesDelaunay3D PRIVATE gdel3d_core CUDA::cudart)
