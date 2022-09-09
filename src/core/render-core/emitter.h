#pragma once
#include "core/utils/configurable.h"
#include "core/render-core/texture.h"
#include "core/mesh/mesh.h"

class Emitter;
class ShapeInterface;

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
        EEnvironment
    } emitter_type;
    Point3f point_on_emitter;
    Ray3f shadow_ray;
    SpectrumRGB energy;
    float pdf;
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

    //TODO, old function
    virtual void sample (PointQueryRecord* pRec, Point2f sample) const = 0;

    virtual void setTexture(Texture *envmap) = 0;

    virtual void sample(DirectIlluminationRecord *d_rec, Point2f sample) const = 0;

    virtual float pdf(const Ray3f &ray) const = 0;
protected:
    Mesh *m_mesh = nullptr;
};