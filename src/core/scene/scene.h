#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <stack>
#include <embree3/rtcore.h>
#include <core/shape/shape.h>
#include <variant>
#include "core/render-core/medium.h"

using Intersection = std::variant<ShapeIntersection,
                                  MediumIntersection>;
class Scene {
public:
    Scene();

    ~Scene() = default;

    void addShape(std::shared_ptr<ShapeInterface> shape);

    void addEmitter(std::shared_ptr<Emitter> emitter);

    void postProcess();

    std::optional<ShapeIntersection> intersect(const Ray3f &ray) const;

    bool occlude(const Ray3f &ray) const;

    SpectrumRGB evaluateEnvironment(const Ray3f &ray) const;
/*
//todo delete
    void sampleAreaIllumination(DirectIlluminationRecord *dRec,
                                Point3f from,
                                Sampler *sampler) const { }
//todo delete
    void sampleAttenuatedAreaIllumination(DirectIlluminationRecord *dRec,
                                          SpectrumRGB *trans,
                                          Point3f from,
                                          std::shared_ptr<Medium> medium,
                                          Sampler *sampler) const { }
//todo deletes
    float pdfAreaIllumination (const ShapeIntersection &its,
                               const Ray3f &ray) const { }
//todo delete
    SpectrumRGB evaluateTrans(std::shared_ptr<Medium> medium,
                              Point3f from,
                              Point3f end) const { }
*/
/*
    std::optional<ShapeIntersection> intersect(const Ray3f &ray,
                                               std::shared_ptr<Medium> medium, 
                                               SpectrumRGB *trans) const;
*/
    void setEnvMap(std::shared_ptr<Emitter> _environment) {
        environment = _environment;
    }
/*
//todo delete
    void sampleEnvironment(DirectIlluminationRecord *dRec,
                           Point3f from,
                           Point2f sample) const { }
//todo delete
    float pdfEnvironment(const Ray3f &ray) const { }
*/

    bool hasEnvironment() const {
        return environment != nullptr;
    }

    std::shared_ptr<Medium> getEnvMedium() const {
        return envMedium;
    }

    void setEnvMedium(std::shared_ptr<Medium> medium) {
        envMedium = medium;
    }

    /* 
        todo from should be replace with an info struct to do get a better sample 
    */    
    void sampleDirectLumin(DirectIlluminationRecord *dRec,
                           Point3f from,
                           Sampler *sampler) const;

    float pdfEmitter(std::shared_ptr<Emitter> emitter) const;

    std::shared_ptr<Emitter> getEnvEmitter() const {
        return environment;
    }
private:
    //* Embree 
    RTCDevice device;
    RTCScene scene;

    int shapeCount = 0;
    std::shared_ptr<Emitter> environment;
    std::vector<std::shared_ptr<ShapeInterface>>  shapes;
    std::shared_ptr<Medium> envMedium;

    std::vector<std::shared_ptr<Emitter>> emitters;
    Distrib1D<std::shared_ptr<Emitter>> lightDistrib;

};