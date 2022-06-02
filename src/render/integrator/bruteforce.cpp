#include "core/render-core/integrator.h"

class BruteForce : public Integrator {
    
public:
    BruteForce(const rapidjson::Value &_value) {
        // do nothing
    }

    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const {
        RayIntersectionRec iRec;
        // miss the scene, return the env evaluation
        if (!scene.rayIntersect(ray, iRec)) {
            //TODO return scene.evalEnvLight(ray);
        }
        

    }

};

REGISTER_CLASS(BruteForce, "bruteforce")