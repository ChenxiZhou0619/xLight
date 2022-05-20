#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include "core/math/math.h"
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "core/render-core/camera.h"
#include "core/file/image.h"
#include "core/render-core/scene.h"

#include <iostream>

void render(const Scene &scene,const Camera &camera ,Image &img) {
    ImageBlock block {Vector2i {0, 0}, img.getSize()};

    for (int i = 0; i < block.getWidth(); ++i) {
        for (int j = 0; j < block.getHeight(); ++j) {
            Ray3f ray = camera.sampleRay (Vector2f {i / (float)img.getWidth(), j / (float)img.getHeight()});
            RayIntersectionRec iRec;
            if (scene.rayIntersect(ray, iRec)){
                SpectrumRGB color {
                    iRec.geoN.x,
                    iRec.geoN.y,
                    iRec.geoN.z
                };
                block.setPixel ( Vector2i {i, j}, color);
            }
            else block.setPixel (Vector2i {i, j}, SpectrumRGB {.0f, .0f, .0f});
        } 
    }

    img.putBlock(block);
}

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

    Scene scene ("/mnt/renderer/xLight/data/monkey.obj");

    PerspectiveCamera camera = PerspectiveCamera(
        Point3f  (0, 0, 2.5),
        Point3f  (0, 0, 0),
        Vector3f (0, 1, 0)
    );

    render(scene, camera, img);

    img.savePNG("test1.png");
    
}

int main(int argc, char **argv) {
    cameraTest();
}