#include "core/mesh/mesh.h"
#include "core/mesh/meshLoader.h"
#include "core/mesh/meshSet.h"
#include "core/math/math.h"
#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "core/render-core/camera.h"
#include "core/file/image.h"
#include "core/render-core/scene.h"

#include "render/integrator/ao.h"
#include "render/integrator/simple.h"
#include "render/sampler/independent.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <tbb/tbb.h>

#include <iostream>
#include <chrono>
#include <thread>


void renderBlock(const Scene &scene,const Camera &camera ,ImageBlock &block, const Vector2i &totalSize) {
    Integrator *integrator = new SimpleIntegrator(Point3f {0, 1, 0}, SpectrumRGB {300.f});
    Sampler *sampler = new Independent();
    
    for (int i = 0; i < block.getWidth(); ++i) {
        for (int j = 0; j < block.getHeight(); ++j) {
            SpectrumRGB color {.0f};

            for (int spp = 0; spp < 4; ++spp) {
                Ray3f ray = camera.sampleRay (
                    Vector2i {i + block.getOffset().x, j + block.getOffset().y},
                    totalSize,
                    sampler->next2D()
                );
            
                color += integrator->getLi(scene, ray);
            }
            color = color / 4;
            
            block.setPixel(Vector2i {i, j}, color);
        } 
    }
    delete integrator;
    delete sampler;
}


void render(const Scene &scene,const Camera &camera ,Image &img) {

    ImageBlockManager blcMng(img.getSize());

    tbb::parallel_for(
        tbb::blocked_range2d<size_t>(0, blcMng.getSize().x, 0, blcMng.getSize().y), 
        [&](const tbb::blocked_range2d<size_t> &r){
                for (size_t rows = r.rows().begin(); rows != r.rows().end(); ++rows)
                    for (size_t cols = r.cols().begin(); cols != r.cols().end(); ++cols){        
                        renderBlock(scene, camera, blcMng.at(rows, cols), img.getSize());
                        img.putBlock(blcMng.at(rows, cols));
                    }
            }
    );

}


void renderTest(const char* filePath) {

    Image img {Vector2i {768, 416}};

    MeshLoader loader;

    MeshSet *meshSetPtr = loader.loadFromFile(filePath);
    Scene scene (meshSetPtr);
    scene.preprocess();

    PerspectiveCamera camera = PerspectiveCamera(
        Point3f  (0, 0, 0),
        Point3f  (0, 0, 1),
        Vector3f (0, 1, 0)
    );

    render(scene, camera, img);
    img.savePNG("dragon-test5.png");
}


int main(int argc, char **argv) {
    renderTest(argv[1]);
}