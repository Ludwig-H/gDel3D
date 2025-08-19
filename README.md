# gDel3D

gDel3D constructs the Delaunay Triangulation of a set of points in 3D on the GPU. This repository refactors the original code so it compiles on recent CUDA toolkits and architectures.

Original repository: https://github.com/ashwin/gDel3D

## Programs

Two executables are produced by the build:

* **gflip3d** – original demo.
* **EdgesDelaunay3D** – extracts the edges of the triangulation.

## Building (Linux)

The project uses CMake and the CUDA Toolkit.

```bash
mkdir build
cmake -S . -B build
cmake --build build
./build/gflip3d          # run the demo
./build/EdgesDelaunay3D  # optional edge extractor
```

Tested on Ubuntu 22.04 with CUDA 12.5. A Visual Studio 2012 project is provided for Windows users.

## Notes

* To switch from double to single precision, define `REAL_TYPE_FP32`.
* For details on input and output see:
  * `CommonTypes.h` (near the end)
  * `Demo.cpp`
  * `DelaunayChecker.cpp`
* A Colab notebook [`gdel3d_colab.ipynb`](gdel3d_colab.ipynb) is available to build and run the project in the cloud.

## Authors

* Cao Thanh Tung
* Ashwin Nanjappa
