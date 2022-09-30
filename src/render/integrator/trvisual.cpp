#include "core/render-core/integrator.h"
#include "core/render-core/medium.h"
#include "core/math/common.h"
#include <stack>


class TrVisual : public Integrator {
public:
    TrVisual() { }

    TrVisual(const rapidjson::Value &_value) {
 
    }
    virtual SpectrumRGB getLi(const Scene &scene,
                              const Ray3f &_ray,
                              Sampler *sampler) const override
    {

        Ray3f ray{_ray};

        auto itsOpt = scene.intersect(ray);
        bool foundIntersection = itsOpt.has_value();

        if (!foundIntersection) {
            return scene.evaluateEnvironment(ray);
        }

        ShapeIntersection its = itsOpt.value();
        auto shape = its.shape;

        if (shape->isEmitter() || shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
            return SpectrumRGB(.0f);
        }

        auto medium = shape->getInsideMedium();
        ray = Ray3f{its.hitPoint, ray.dir};

        auto endIts = scene.intersect(ray);
        if (!endIts.has_value())
            return SpectrumRGB(.0f);

        auto tr = medium->getTrans(its.hitPoint, endIts->hitPoint);

        return scene.evaluateEnvironment(ray) * tr;

    }
private:
};

REGISTER_CLASS(TrVisual, "trvisual")