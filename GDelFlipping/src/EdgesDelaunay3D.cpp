#include <fstream>
#include <iostream>
#include <set>
#include <utility>

#include "gDel3D/GpuDelaunay.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " input.xyz output.txt\n";
        return 1;
    }
    const char *inFile = argv[1];
    const char *outFile = argv[2];

    std::ifstream in(inFile);
    if (!in) {
        std::cerr << "Failed to open input file\n";
        return 1;
    }

    Point3HVec points;
    double x, y, z;
    while (in >> x >> y >> z) {
        Point3 p;
        p._p[0] = x;
        p._p[1] = y;
        p._p[2] = z;
        points.push_back(p);
    }
    in.close();

    if (points.empty()) {
        std::cerr << "No points loaded from input file\n";
        return 1;
    }

    GDelOutput output;
    GpuDel triangulator;
    triangulator.compute(points, &output);

    const int infIdx = static_cast<int>(points.size());
    std::set<std::pair<int, int>> edges;

    for (const Tet &t : output.tetVec) {
        int v[4] = {t._v[0], t._v[1], t._v[2], t._v[3]};
        for (int i = 0; i < 4; ++i) {
            for (int j = i + 1; j < 4; ++j) {
                int a = v[i];
                int b = v[j];
                if (a == infIdx || b == infIdx) {
                    continue;
                }
                if (a > b) {
                    std::swap(a, b);
                }
                edges.emplace(a, b);
            }
        }
    }

    std::ofstream out(outFile);
    if (!out) {
        std::cerr << "Failed to open output file\n";
        return 1;
    }
    for (const auto &e : edges) {
        out << e.first << " " << e.second << '\n';
    }
    out.close();

    return 0;
}
