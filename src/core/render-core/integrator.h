#pragma once
#include "core/utils/configurable.h"
#include "bsdf.h"
#include "sampler.h"
#include "emitter.h"
#include "core/scene/scene.h"
#include <variant>
#include "medium.h"

class Scene;
class RenderTask;
class ImageBlock;

class Integrator : public Configurable {
public:
    Integrator() = default;
    Integrator(const rapidjson::Value &_value) {}
    
    virtual ~Integrator() {}
    virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray, Sampler *sampler) const = 0;

    virtual void render(std::shared_ptr<RenderTask> task) const;
protected:
    virtual void render_block(std::shared_ptr<ImageBlock> block, std::shared_ptr<RenderTask> task) const;
};
