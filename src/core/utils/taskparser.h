#pragma once
#include "core/render-core/scene.h"
#include "core/render-core/sampler.h"
#include "core/render-core/integrator.h"
class Image;

struct RenderTask {
    std::unique_ptr<Image>      image       {nullptr};
    std::unique_ptr<Scene>      scene       {nullptr};
    std::unique_ptr<Sampler>    sampler     {nullptr};
    std::unique_ptr<Camera>     camera      {nullptr};
    std::unique_ptr<Integrator> integrator  {nullptr};

    Vector2i getImgSize() const ;

    uint32_t getSpp() const ;

};

struct RenderTaskParser {
    static std::unique_ptr<RenderTask> createTask(const std::string &filename);
};