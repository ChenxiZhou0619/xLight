#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include "core/geometry/geometry.h"

#include <iostream>

void meshTest(int argc, char **argv) {
    if (1 == argc) {
        std::cerr << "Arguments num error!\n";
        exit(0); 
    }
    MeshLoader loader;

    auto meshSet = loader.loadFromFile(argv[1]);

    std::cout << meshSet->toString() << std::endl;

    int meshIdx = 0, triIdx = 0;

    auto fBuf = meshSet->getFBuf(meshIdx, triIdx);
    std::cout << fBuf << std::endl;

    std::cout << "p0 = " << meshSet->getVtxBuf(meshIdx, fBuf[0]) << std::endl;
    std::cout << "p1 = " << meshSet->getVtxBuf(meshIdx, fBuf[1]) << std::endl;
    std::cout << "p2 = " << meshSet->getVtxBuf(meshIdx, fBuf[2]) << std::endl;
    
    std::cout << "n0 = " << meshSet->getNmlBuf(meshIdx, fBuf[0]) << std::endl;
    std::cout << "n1 = " << meshSet->getNmlBuf(meshIdx, fBuf[1]) << std::endl;
    std::cout << "n2 = " << meshSet->getNmlBuf(meshIdx, fBuf[2]) << std::endl;

    std::cout << meshSet->getAABB3(meshIdx) << std::endl;

}

int main(int argc, char **argv) {
    meshTest(argc, argv);
}