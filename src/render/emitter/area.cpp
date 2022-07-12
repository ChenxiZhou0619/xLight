#include "core/render-core/emitter.h"

class AreaEmitter : public Emitter {
    // TODO, replace it with texture
    SpectrumRGB m_lightEnergy;
public:
    AreaEmitter() = default;
    AreaEmitter(const rapidjson::Value &_value) {
        m_lightEnergy = getSpectrumRGB("lightEnergy", _value);
    }
    ~AreaEmitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const override{
        return m_lightEnergy;
    }

    virtual SpectrumRGB evaluate(const Ray3f &ray) const override{
        return m_lightEnergy;
    }

    virtual void sample(PointQueryRecord *pRec, Point2f sample) const override {
        //! no implement
        std::cout << "AreaEmitter::sample no implement!\n";
        std::exit(1);
    }

    virtual void setTexture(Texture *texture) override {
        //! no implement
        std::cout << "AreaEmitter::setTexture no implement!\n";
        std::exit(1);
    }

    virtual float pdf(const Ray3f &ray) const override {
        //! no implement
        std::cout << "Error, area pdf not implement!\n";
        std::exit(1);
    }

    virtual void sample(DirectIlluminationRecord *d_rec, Point2f sample) const override{
        //! no implement
        std::cout << "AreaEmitter::sample2 no implement!\n";
        std::exit(1);
    }
};

REGISTER_CLASS(AreaEmitter, "area")