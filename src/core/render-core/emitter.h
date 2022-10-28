#pragma once
#include "core/utils/configurable.h"
#include "core/render-core/texture.h"
#include "core/mesh/mesh.h"
#include <optional>

class Emitter;
class ShapeInterface;
struct LightSourceInfo;
struct SurfaceIntersectionInfo;
struct IntersectionInfo;

struct PointQueryRecord {
    Point3f p;
    Normal3f normal;
    float pdf;
    // TODO delete this
    const Mesh *mesh;
    const Emitter *emitter;

    std::shared_ptr<ShapeInterface> shape;
};

struct EmitterQueryRecord {
    PointQueryRecord pRec;
    Ray3f ray;

    EmitterQueryRecord() = default;
    EmitterQueryRecord(const PointQueryRecord &_pRec, const Ray3f &_ray):pRec(_pRec), ray(_ray) { }

    const Emitter *getEmitter() const {
        if (pRec.mesh == nullptr)
            return pRec.emitter;
        return pRec.mesh->getEmitter();
    }
};

struct DirectIlluminationRecord {
    enum class EmitterType {
        EArea = 0,
        EEnvironment,
        ESpot,
    } emitter_type;
    Ray3f shadow_ray;
    SpectrumRGB energy;
    float pdf;
    bool isDelta = false;
};

struct EmitterHitInfo{
    float dist;
    Point3f hitpoint;
    Normal3f normal;
    Vector3f dir;
};

// TODO, setTexture method, and maybe Emitter holds a mesh / entity is a good choice
class Emitter : public Configurable {
public:
    Emitter() = default;
    Emitter(const rapidjson::Value &_value);
    virtual ~Emitter() = default;

    virtual void initialize() {
        // do nothing
    }

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const = 0;

    virtual SpectrumRGB evaluate(const Ray3f &ray) const = 0;

    virtual SpectrumRGB evaluate(const LightSourceInfo &info,
                                 Point3f destination) const = 0;

    virtual SpectrumRGB evaluate(const SurfaceIntersectionInfo &itsInfo) const = 0;

    virtual float pdf(const SurfaceIntersectionInfo &info) const = 0;

    virtual LightSourceInfo sampleLightSource(const IntersectionInfo &info, 
                                              Point3f sample) const = 0;

    //TODO, old function
    virtual void sample (PointQueryRecord* pRec, Point2f sample) const = 0;

    virtual void setTexture(Texture *envmap) = 0;

    virtual void sample(DirectIlluminationRecord *d_rec, 
                        Point3f sample, 
                        Point3f position) const = 0;

    virtual std::pair<Point3f, float> samplePoint(Point3f sample) const = 0;

    virtual float pdf(const EmitterHitInfo &info)const = 0;

    std::weak_ptr<ShapeInterface> shape;
};