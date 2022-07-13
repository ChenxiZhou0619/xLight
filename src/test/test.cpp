#include "core/file/image.h"
#include "core/render-core/integrator.h"
#include "core/render-core/sampler.h"
#include "core/render-core/camera.h"
#include "core/render-core/scene.h"
#include "core/utils/taskparser.h"

#include <tbb/tbb.h>
#include <tinyformat/tinyformat.h>
#include <iostream>
#include <chrono>


void renderBlock(ImageBlock &block, const RenderTask* task) {
    Integrator *integrator = task->integrator.get();
    std::shared_ptr<Sampler> sampler = task->sampler->clone();
    Camera *camera = task->camera.get();
    Scene *scene = task->scene.get();

    float percentage = .0f;
    for (int i = 0; i < block.getWidth(); ++i) {
        for (int j = 0; j < block.getHeight(); ++j) {
            SpectrumRGB color {.0f};
            Point2f pixel = Point2f (i + block.getOffset().x, j + block.getOffset().y);
            sampler->startPixel(pixel);
            for (int spp = 0; spp < task->getSpp(); ++spp) {
                Ray3f ray = camera->sampleRay (
                    Vector2i {i + block.getOffset().x, j + block.getOffset().y},
                    task->getImgSize(),
                    sampler->getCameraSample()
                );            
                color += integrator->getLi(*scene, ray, sampler.get());
                sampler->nextSample();
            }
            color = color / (float)task->getSpp();
            
            block.setPixel(Vector2i {i, j}, color);
        }
    }
}


void render(const RenderTask* task) {
    auto start = std::chrono::high_resolution_clock::now();

    ImageBlockManager blcMng(task->getImgSize());
    int finished = 0;
    
    tbb::parallel_for(
        tbb::blocked_range2d<size_t>(0, blcMng.getSize().x, 0, blcMng.getSize().y), 
        [&](const tbb::blocked_range2d<size_t> &r){
                for (size_t rows = r.rows().begin(); rows != r.rows().end(); ++rows)
                    for (size_t cols = r.cols().begin(); cols != r.cols().end(); ++cols){        
                        renderBlock(
                            blcMng.at(rows, cols), 
                            task
                        ); 
                        task->image->putBlock(blcMng.at(rows, cols));
                    }
                ++finished;
                std::cout << tfm::format("Finish : %.2f\n", (float)finished / (blcMng.getSize().x * blcMng.getSize().y));
            }
    );
    
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << tfm::format("Rendering costs : %.2f seconds\n",
        (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f);
    task->image->savePNG();
}


int main(int argc, char **argv) {
    if (!argv[1]) {
        std::cout << "No given file!\n";
        exit(1);
    }
    std::unique_ptr<RenderTask> task = RenderTaskParser::createTask(argv[1]);
    std::cout << "Start to render\n";
    render(task.get());
}