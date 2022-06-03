#include "core/render-core/emitter.h"

class AreaEmitter : public Emitter {
    SpectrumRGB m_lightEnergy;
public:
    AreaEmitter() = default;
    AreaEmitter(const rapidjson::Value &_value) {
        m_lightEnergy = getSpectrumRGB("lightEnergy", _value);
    }
    ~AreaEmitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const {
        return m_lightEnergy;
    }

    virtual SpectrumRGB evaluate(const Ray3f &ray) const {
        return m_lightEnergy;
    }
};

REGISTER_CLASS(AreaEmitter, "area")