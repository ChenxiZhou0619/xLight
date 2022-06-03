#pragma once
#include "core/mesh/meshLoader.h"
#include "core/geometry/geometry.h"
#include "core/accelerate/accel.h"
#include "core/math/discretepdf.h"
#include "camera.h"
#include <memory>


class Scene {
public:
    Scene() = default;

    Scene(MeshSet *_meshSetPtr) : accelPtr(std::make_unique<Accel>(_meshSetPtr)) {}

    void preprocess();

    bool rayIntersect(const Ray3f &ray) const ;

    bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const ;

    void sampleEmitterOnSurface(PointQueryRecord &pRec, Sampler *sampler) const;   

private:
    std::unique_ptr<Accel> accelPtr;
    
    std::vector<Mesh *> emitterList;
    DiscretePDF emitterDistribution;
    float emitterSurfaceArea;
};