#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include <iostream>

int main(int argc, char **argv) {
    if (1 == argc) {
        std::cerr << "Arguments num error!\n";
        exit(0); 
    }
    MeshLoader loader;

    auto meshPtr = loader.loadMesh(argv[1]);

    return 0;
}