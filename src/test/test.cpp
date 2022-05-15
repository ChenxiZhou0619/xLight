#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"

#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "core/file/image.h"


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

void spectrumTest() {
    SpectrumRGB rgb1 (Vector3f(1.0f, .5f, .1f));
    std::cout << rgb1.toString() << std::endl;
    SpectrumRGB rgb2 (Vector3f(.2f, .3f, .0f));
    std::cout << (rgb1 + rgb2).toString() << std::endl;
    std::cout << (rgb1 - rgb2).toString() << std::endl;
    std::cout << (rgb1 * rgb2).toString() << std::endl;
    std::cout << (rgb1 / rgb2).toString() << std::endl;
}

void imageTest() {

}

int main(int argc, char **argv) {
    Image img(Vector2i(5,5));
    ImageBlock block(Vector2i(1, 2), Vector2i(2, 2));
    block.setPixel(Vector2i(1, 1), SpectrumRGB(.1f));
    block.setPixel(Vector2i(0, 1), SpectrumRGB(.2f));
    std::cout << block << std::endl;
    img.putBlock(block);
    std::cout << img << std::endl;
}