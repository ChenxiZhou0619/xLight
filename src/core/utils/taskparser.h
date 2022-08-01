#pragma once
#include "core/render-core/scene.h"
#include "core/render-core/sampler.h"
#include "core/render-core/integrator.h"
#include "core/render-core/bsdf.h"
#include "core/render-core/emitter.h"
#include "core/render-core/medium.h"
class Image;

struct RenderTask {
    std::unique_ptr<Image>      image       {nullptr};
    std::unique_ptr<Scene>      scene       {nullptr};
    std::unique_ptr<Sampler>    sampler     {nullptr};
    std::unique_ptr<Camera>     camera      {nullptr};
    std::unique_ptr<Integrator> integrator  {nullptr};

    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    std::unordered_map<std::string, std::unique_ptr<BSDF>> bsdfs;
    std::unordered_map<std::string, std::unique_ptr<Emitter>> emitters;
    std::unordered_map<std::string, std::unique_ptr<Medium>> mediums;

    Vector2i getImgSize() const ;

    uint32_t getSpp() const ;

    Texture* getTexture(const std::string &textureName) const;

};

struct RenderTaskParser {
    static std::unique_ptr<RenderTask> createTask(const std::string &filename);
};