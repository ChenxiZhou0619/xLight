#pragma once
#include "core/utils/configurable.h"
#include "core/mesh/mesh.h"
#include "core/render-core/texture.h"

class Emitter;

struct PointQueryRecord {
    Point3f p;
    Normal3f normal;
    float pdf;
    const Mesh *mesh;
    const Emitter *emitter;
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

    virtual void sample (PointQueryRecord* pRec, Point2f sample) const = 0;

    virtual void setTexture(Texture *envmap) = 0;
protected:
    Mesh *m_mesh = nullptr;
};