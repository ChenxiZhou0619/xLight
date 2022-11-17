#pragma once

#include "core/scene/scene.h"
#include "core/render-core/sampler.h"
#include "core/render-core/integrator.h"
#include "core/render-core/bsdf.h"
#include "core/render-core/emitter.h"
#include "core/render-core/medium.h"
#include "core/render-core/camera.h"
#include "core/file/image.h"
#include "core/file/figure.h"

struct RenderTask {
    std::shared_ptr<Image>      image;
    std::shared_ptr<Scene>      scene;
    std::shared_ptr<Sampler>    sampler;
    std::shared_ptr<Camera>     camera;
    std::shared_ptr<Integrator> integrator;

    std::unordered_map<std::string, std::shared_ptr<Texture>>   textures;
    std::unordered_map<std::string, std::shared_ptr<BSDF>>      bsdfs;
    std::unordered_map<std::string, std::shared_ptr<Emitter>>   emitters;
    std::unordered_map<std::string, std::shared_ptr<Medium>>    mediums;

    std::shared_ptr<Texture> getTexture(const std::string &textureName) const;
    std::shared_ptr<BSDF> getBSDF(const std::string &bsdfName) const;
    std::shared_ptr<Emitter> getEmitter(const std::string &emitterName) const;
    std::shared_ptr<Medium> getMedium(const std::string &mediumName) const;


    RenderTask() {
        scene = std::make_shared<Scene>();
    }

    Vector2i getImgSize() const;

    int getSpp() const;

};

std::shared_ptr<RenderTask> createTask(const std::string &filepath);