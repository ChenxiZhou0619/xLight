#include <iostream>
#include <core/shape/shape.h>
#include <core/shape/mesh.h>
#include <core/scene/scene.h>
#include <core/task/task.h>

#include <chrono>
#include <tbb/tbb.h>
#include <mutex>

void renderBlock(std::shared_ptr<ImageBlock> block, std::shared_ptr<RenderTask> task) {
    //Integrator *integrator = task->integrator.get();
    auto integrator = task->integrator;
    std::shared_ptr<Sampler> sampler = task->sampler->clone();
    //Camera *camera = task->camera.get();
    //Scene *scene = task->scene.get();
    auto camera = task->camera;
    auto scene =  task->scene;

    for (int i = 0; i < block->getWidth(); ++i) {
        for (int j = 0; j < block->getHeight(); ++j) {
            SpectrumRGB color {.0f};
            auto [x, y] = block->getOffset();
            Point2f pixel = Point2f (i + x, j + y);
            sampler->startPixel(pixel);
            for (int spp = 0; spp < task->getSpp(); ++spp) {
                Ray3f ray = camera->sampleRayDifferential (
                    Vector2i {i + x, j + y},
                    task->getImgSize(),
                    sampler->getCameraSample()
                );  
                color += integrator->getLi(*scene, ray, sampler.get());

                sampler->nextSample();
            }
            color = color / (float)task->getSpp();
            block->setPixel(Vector2i {i, j}, color);
        }
    }
}

void render(std::shared_ptr<RenderTask> task) {
    auto start = std::chrono::high_resolution_clock::now();
    //ImageBlockManager blcMng(task->getImgSize());
    
    BlockManager bm{task->image->getSize(), 32};
    //int finished = 0;
    auto [x, y] = bm.getSize();
    
    std::vector<std::shared_ptr<ImageBlock>> resultBlocks;
    resultBlocks.reserve(x * y);

    double i = 0;
    double total = x * y;
    tbb::parallel_for(
        tbb::blocked_range2d<size_t>(0, x, 0, y), 
        [&](const tbb::blocked_range2d<size_t> &r){
                for (size_t rows = r.rows().begin(); rows != r.rows().end(); ++rows)
                    for (size_t cols = r.cols().begin(); cols != r.cols().end(); ++cols){        
                        auto blc = bm.getBlock(rows, cols);
                        renderBlock(
                            blc, 
                            task
                        ); 
                        resultBlocks.emplace_back(blc);
                        i++;
                        if ((int)i % 10 == 0) {
                            std::cout << "Finish " << (int)(i * 100/total) << "%\n";
                        }
                    }
            }
    );

    for (auto i : resultBlocks) {
        task->image->putBlock(*i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << tfm::format("Rendering costs : %.2f seconds\n",
        (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f);
    task->image->savePNG();
}



int main(int argc, char **argv) {
    auto task = createTask(argv[1]);
    render(task);
}