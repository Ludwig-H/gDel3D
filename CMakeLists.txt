diff --git a/CMakeLists.txt b/CMakeLists.txt
index e2d7b152d6b60560bef2922dc8a86bf46b90a43c..eec8605e92a4b5eb3aa205d36b5527733edbb9a3 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -1,68 +1,88 @@
-cmake_minimum_required(VERSION 3.21)
+cmake_minimum_required(VERSION 3.18)
 
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
 
+# Bibliothèque centrale contenant toutes les implémentations CPU/GPU
+add_library(gdel3d_core ${GDEL3D_CORE})
+target_include_directories(gdel3d_core PUBLIC
+  ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
+)
+
+# Détecter et cibler automatiquement l'arch du GPU
+if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
+  set(CUDA_ARCH native)
+else()
+  execute_process(
+    COMMAND nvidia-smi --query-gpu=compute_cap --format=csv,noheader
+    OUTPUT_VARIABLE GPU_CAP
+    RESULT_VARIABLE GPU_CAP_RESULT
+    OUTPUT_STRIP_TRAILING_WHITESPACE
+  )
+  if(GPU_CAP_RESULT EQUAL 0 AND GPU_CAP)
+    string(REPLACE "." "" CUDA_ARCH ${GPU_CAP})
+  else()
+    # Valeur par défaut raisonnable (GPU Turing)
+    set(CUDA_ARCH 75)
+  endif()
+endif()
+
+set_target_properties(gdel3d_core PROPERTIES
+  CUDA_ARCHITECTURES ${CUDA_ARCH}
+  CUDA_SEPARABLE_COMPILATION ON
+  POSITION_INDEPENDENT_CODE ON
+)
+
+# Option utile si Thrust/CUB se plaint de versions et forcer le backend C++
+# pour les fichiers compilés avec le compilateur hôte afin d'éviter
+# l'inclusion de CUB qui provoque des erreurs lorsqu'il n'est pas
+# compilé par NVCC.
+target_compile_definitions(gdel3d_core PUBLIC
+  THRUST_IGNORE_CUB_VERSION_CHECK=1
+  $<$<COMPILE_LANGUAGE:CXX>:THRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP>
+)
+
+# Eviter que CMake impose -Werror=deprecated-declarations via toolchains exotiques
+if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
+  target_compile_options(gdel3d_core PRIVATE -Wno-deprecated-declarations)
+endif()
+if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
+  target_compile_options(gdel3d_core PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>)
+endif()
+
 # Executable de démonstration original
 add_executable(gflip3d
-  ${GDEL3D_CORE}
   GDelFlipping/src/Demo.cpp
   GDelFlipping/src/DelaunayChecker.cpp
   GDelFlipping/src/InputCreator.cpp
   GDelFlipping/src/RandGen.cpp
 )
+target_link_libraries(gflip3d PRIVATE gdel3d_core $<LINK_ONLY:CUDA::cudart>)
 
-# Nouveau programme pour extraire les arêtes du triangulation
+# Nouveau programme pour extraire les arêtes de la triangulation
 add_executable(EdgesDelaunay3D
-  ${GDEL3D_CORE}
   GDelFlipping/src/EdgesDelaunay3D.cpp
 )
-
-foreach(target IN ITEMS gflip3d EdgesDelaunay3D)
-  target_include_directories(${target} PRIVATE
-    ${CMAKE_CURRENT_SOURCE_DIR}/GDelFlipping/src
-  )
-
-  # Compiler avec NVCC et lier le runtime CUDA
-  target_link_libraries(${target} PRIVATE CUDA::cudart)
-
-  # Détecter et cibler automatiquement l'arch du GPU
-  set_target_properties(${target} PROPERTIES
-    CUDA_ARCHITECTURES native
-    CUDA_SEPARABLE_COMPILATION ON
-    POSITION_INDEPENDENT_CODE ON
-  )
-
-  # Option utile si Thrust/CUB se plaint de versions
-  target_compile_definitions(${target} PRIVATE THRUST_IGNORE_CUB_VERSION_CHECK=1)
-
-  # Eviter que CMake impose -Werror=deprecated-declarations via toolchains exotiques
-  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
-    target_compile_options(${target} PRIVATE -Wno-deprecated-declarations)
-  endif()
-  if (CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA")
-    target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-fPIC>)
-  endif()
-endforeach()
+target_link_libraries(EdgesDelaunay3D PRIVATE gdel3d_core $<LINK_ONLY:CUDA::cudart>)
