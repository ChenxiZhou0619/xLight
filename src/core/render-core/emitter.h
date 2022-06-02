#pragma once
#include "core/utils/configurable.h"

struct PointQueryRecord {
    Point3f point;
    float pdf;
};

struct EmitterQueryRecord {
    PointQueryRecord pRec;
    Vector3f dir;
};

class Emitter : public Configurable {
public:
    Emitter() = default;
    Emitter(const rapidjson::Value &_value);
    ~Emitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const = 0;
};