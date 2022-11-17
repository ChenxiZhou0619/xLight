#include <chrono>

#include <core/task/task.h>
#include <core/render-core/integrator.h>

#include <tbb/tbb.h>

class BidirectionalPathTracer : public Integrator {
public:
    BidirectionalPathTracer() : mMaxDepth(5) { }

    BidirectionalPathTracer(const rapidjson::Value &_value)
    {
        mMaxDepth = getInt("maxDepth", _value);
    }

    virtual ~BidirectionalPathTracer() = default;

    virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                              Sampler *sampler) const override
    {
        // no implementation
        std::cerr << "BidirectionalPathTracer::getLi not implement!\n";
        std::exit(1);
    }

    virtual void render(std::shared_ptr<RenderTask> task) const override{
        auto start_time = std::chrono::high_resolution_clock::now();
        
        BlockManager bm = {task->image->getSize(), 32};
        auto [x, y] = bm.getSize();

        std::vector<std::shared_ptr<ImageBlock>> result_blocks;
        result_blocks.reserve(x * y);

        int finished_tiles = 0;
        int total = x * y;
        tbb::parallel_for(
            tbb::blocked_range2d<size_t>(0, x, 0, y),
            [&](const tbb::blocked_range2d<size_t> &r) {
                for (size_t row = r.rows().begin(); row != r.rows().end(); ++row) {
                    for (size_t col = r.cols().begin(); col != r.cols().end(); ++col) {
                        auto tile = bm.getBlock(row, col);
                        render_block(tile, task);
                        result_blocks.emplace_back(tile);
                        finished_tiles++;
                        if (finished_tiles % 5 == 0) {
                            printProgress((double)finished_tiles / total);
                        }
                    }
                }
            }
        );
        printProgress(1);
        for (auto i : result_blocks) task->image->putBlock(*i);
        auto finish_time = std::chrono::high_resolution_clock::now();
        std::cout << tfm::format("\nRendering costs : %.2f seconds\n", (float)std::chrono::duration_cast<std::chrono::milliseconds>(finish_time-start_time).count() / 1000.f);
        task->image->savePNG();
    }

private:
    int mMaxDepth;

    virtual void render_block(std::shared_ptr<ImageBlock> block, std::shared_ptr<RenderTask> task) const override
    {
        
    }
};

REGISTER_CLASS(BidirectionalPathTracer, "bdpt")