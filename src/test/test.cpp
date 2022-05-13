#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include <iostream>

int main(int argc, char **argv) {
    if (1 == argc) {
        std::cerr << "Arguments num error!\n";
        exit(0); 
    }
    MeshLoader loader;

    auto meshSet1 = loader.loadFromFile(argv[1]);
    std::cout << meshSet1->toString() << std::endl;

    auto meshSet2 = loader.loadFromFile(argv[2]);
    std::cout << meshSet2->toString() << std::endl;

    meshSet1->mergeMeshSet(std::move(meshSet2));
    std::cout << meshSet1->toString() << std::endl;

    if (meshSet2)
        std::cout << meshSet2->toString() << std::endl;

    return 0;
}