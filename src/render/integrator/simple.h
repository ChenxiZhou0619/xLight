#pragma once
#include "integrator.h"

class SimpleIntegrator : public Integrator {
    Point3f m_lightPosition;
    SpectrumRGB m_lightEnergy;
public:
    SimpleIntegrator() : 
        m_lightPosition(Point3f {0, 1, 0}), m_lightEnergy(SpectrumRGB {20.f}) { }

    SimpleIntegrator(const Point3f &_lightPosition, const SpectrumRGB &_lightEnergy) :
        m_lightPosition(_lightPosition), m_lightEnergy(_lightEnergy) { }

    virtual SpectrumRGB getLi(const Scene &scene, const Ray3f &ray) const;

    

};