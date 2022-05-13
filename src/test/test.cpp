#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include <iostream>

#include "core/geometry/geometry.h"

void meshTest(int argc, char **argv) {
        if (1 == argc) {
        std::cerr << "Arguments num error!\n";
        exit(0); 
    }
    MeshLoader loader;

    auto meshSet = loader.loadFromFile(argv[1]);

    std::cout << meshSet->toString() << std::endl;

    int meshIdx = 3, triIdx = 0;

    auto fBuf = meshSet->getFBuf(meshIdx, triIdx);
    std::cout << fBuf << std::endl;

    std::cout << "p0 = " << meshSet->getVtxBuf(meshIdx, fBuf[0]) << std::endl;
    std::cout << "p1 = " << meshSet->getVtxBuf(meshIdx, fBuf[1]) << std::endl;
    std::cout << "p2 = " << meshSet->getVtxBuf(meshIdx, fBuf[2]) << std::endl;
    
    std::cout << "n0 = " << meshSet->getNmlBuf(meshIdx, fBuf[0]) << std::endl;
    std::cout << "n1 = " << meshSet->getNmlBuf(meshIdx, fBuf[1]) << std::endl;
    std::cout << "n2 = " << meshSet->getNmlBuf(meshIdx, fBuf[2]) << std::endl;
}

int main(int argc, char **argv) {
    Ray3f ray {Point3f{1, 1, 1}, Vector3f {-1, 0 , -1}};
    AABB3f box {Point3f{-0.1, -0.1, -0.1}, Point3f{0.1, 0.1, 0.1}};
    std::cout << box.rayIntersect(ray) << std::endl;
}