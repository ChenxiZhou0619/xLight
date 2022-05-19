#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include "core/math/math.h"
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "core/render-core/camera.h"
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

    std::cout << meshSet->getAABB3() << std::endl;

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
    Image img(Vector2i(400, 300));
    
    ImageBlock block{
        Vector2i(100, 0), 
        Vector2i(200, 100)
    };

    for (int i = 0; i < 200; ++i)
        for (int j = 0; j < 100; ++j)
            block.setPixel(Vector2i(i, j), SpectrumRGB {.1f, .6f, .2f});      

    img.putBlock (
        block
    );

    img.savePNG("test.png");
}

void matTest() {
    Mat4f m;
    std::cout << m << std::endl;
}

void cameraTest() {

    Image img {Vector2i {400, 300}};

    PerspectiveCamera camera {
        Point3f  (0, 0, -3),
        Point3f  (0, 0, 0),
        Vector3f (0, 1, 0)
    };

    MeshLoader loader;

    auto meshSet = loader.loadFromFile("/mnt/renderer/xLight/data/rectangle.obj");

    ImageBlock block {Vector2i {0, 0}, Vector2i {400, 300}};

    for (int i = 0; i < 400; ++i) {
        for (int j = 0; j < 300; ++j) {
            Ray3f ray = camera.sampleRay (Vector2f {i / 400.f, j / 300.f});
            if (meshSet->getAABB3().rayIntersect(ray)) {
                RayIntersectionRec iRec;
                if (meshSet->rayIntersectMeshFace(ray, iRec, 0, 0)||meshSet->rayIntersectMeshFace(ray, iRec, 0, 1))
                    block.setPixel ( Vector2i {i, j}, SpectrumRGB { 0.2f, 0.9f, 0.2f});
            }
            else block.setPixel ( Vector2i {i, j}, SpectrumRGB {.1f, .1f, .1f});
        }   
    }

    img.putBlock(block);
    img.savePNG("test3.png");
    
}

int main(int argc, char **argv) {
    cameraTest();
}