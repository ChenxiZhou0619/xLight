#pragma once
#include "core/utils/configurable.h"
#include "core/mesh/mesh.h"

class Emitter;

struct PointQueryRecord {
    Point3f p;
    Normal3f normal;
    float pdf;
    const Mesh *mesh;
};

struct EmitterQueryRecord {
    PointQueryRecord pRec;
    Ray3f ray;

    EmitterQueryRecord() = default;
    EmitterQueryRecord(const PointQueryRecord &_pRec, const Ray3f &_ray):pRec(_pRec), ray(_ray) { }

    Emitter *getEmitter() const {
        return pRec.mesh->getEmitter();
    }
};

class Emitter : public Configurable {
public:
    Emitter() = default;
    Emitter(const rapidjson::Value &_value);
    ~Emitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const = 0;

    virtual SpectrumRGB evaluate(const Ray3f &ray) const = 0;
};