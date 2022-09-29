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
                                          std::shared_ptr<Medium> medium,
                                          Sampler *sampler) const;

    float pdfAreaIllumination (const ShapeIntersection &its,
                               const Ray3f &ray) const;

    SpectrumRGB evaluateTrans(std::shared_ptr<Medium> medium,
                              Point3f from,
                              Point3f end) const;

    std::optional<ShapeIntersection> intersect(const Ray3f &ray,
                                               std::shared_ptr<Medium> medium, 
                                               SpectrumRGB *trans) const;

    std::shared_ptr<Medium> getTargetMedium(Vector3f wo,
                                            const ShapeIntersection &its) const;

    void setEnvMap(std::shared_ptr<Emitter> _environment) {
        environment = _environment;
    }
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