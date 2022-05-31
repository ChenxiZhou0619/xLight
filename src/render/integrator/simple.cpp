#include <iostream>
#include "core/render-core/integrator.h"

class SimpleIntegrator : public Integrator {
    Point3f m_lightPosition;
    SpectrumRGB m_lightEnergy;
public:
    SimpleIntegrator() : 
        m_lightPosition(Point3f {0, 1, 0}), m_lightEnergy(SpectrumRGB {20.f}) { }

    SimpleIntegrator(const Point3f &_lightPosition, const SpectrumRGB &_lightEnergy) :
        m_lightPosition(_lightPosition), m_lightEnergy(_lightEnergy) { }

    SimpleIntegrator(const rapidjson::Value &_value) {
        m_lightPosition = Point3f {
            _value["lightPosition"].GetArray()[0].GetFloat(),
            _value["lightPosition"].GetArray()[1].GetFloat(),
            _value["lightPosition"].GetArray()[2].GetFloat()
        };
        m_lightEnergy = SpectrumRGB {
            _value["lightEnergy"].GetArray()[0].GetFloat(),
            _value["lightEnergy"].GetArray()[1].GetFloat(),
            _value["lightEnergy"].GetArray()[2].GetFloat()
        };
    }

    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const {
        RayIntersectionRec iRec;
        if (!scene.rayIntersect(ray, iRec))
            return SpectrumRGB{.0f};
        Vector3f dir = m_lightPosition - iRec.p;
        Ray3f shadowRay {
            iRec.p, dir, .0f, EPSILON, dir.length()
        };
        //std::cout << shadowRay << std::endl;

        if (!scene.rayIntersect(shadowRay)) {
            SpectrumRGB Li = m_lightEnergy * (.25f * INV_PI * INV_PI)
                * std::max(.0f, dot(iRec.shdN, shadowRay.dir)) / dir.length2();
            //std::cout << Li << std::endl;
            return Li;
        }
        return SpectrumRGB {.0f};
    }
};

REGISTER_CLASS(SimpleIntegrator, "simple")
