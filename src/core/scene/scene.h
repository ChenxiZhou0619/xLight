#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <stack>
#include <embree3/rtcore.h>
#include <core/shape/shape.h>

class Scene {
public:
    Scene();

    ~Scene() = default;

    void addShape(std::shared_ptr<ShapeInterface> shape);

    void postProcess();

    std::optional<ShapeIntersection> intersect(const Ray3f &ray) const;

    bool occlude(const Ray3f &ray) const;

    SpectrumRGB evaluateEnvironment(const Ray3f &ray) const;

    void sampleAreaIllumination(DirectIlluminationRecord *dRec,
                                Point3f from,
                                Sampler *sampler) const;

    void sampleAttenuatedAreaIllumination(DirectIlluminationRecord *dRec,
                                          SpectrumRGB *trans,
                                          Point3f from,
                                          const std::stack<std::shared_ptr<Medium>>& mediums,
                                          Sampler *sampler) const;

    float pdfAreaIllumination (const ShapeIntersection &its,
                               const Ray3f &ray) const;

    SpectrumRGB evaluateTrans(const std::stack<std::shared_ptr<Medium>> &mediums,
                              Point3f from,
                              Point3f end) const;

    std::optional<ShapeIntersection> intersect(const Ray3f &ray,
                                               const std::stack<std::shared_ptr<Medium>>& mediums, 
                                               SpectrumRGB *trans) const;
private:
    //* Embree 
    RTCDevice device;
    RTCScene scene;

    int shapeCount = 0;
    std::shared_ptr<Emitter> environment;
    std::vector<std::shared_ptr<ShapeInterface>>  shapes;

    std::vector<std::shared_ptr<ShapeInterface>>  areaEmitters;
    float emittersSurfaceArea;
    Distribution1D emittersDistribution;
};