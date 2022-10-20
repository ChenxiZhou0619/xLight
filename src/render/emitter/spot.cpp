#include "core/render-core/emitter.h"

class SpotEmitter : public Emitter {
public:
    SpotEmitter() = default;

    SpotEmitter(const rapidjson::Value &_value) {
        lightEnergy = getSpectrumRGB("lightEnergy", _value);
        position = getPoint3f("position", _value);
    }

    virtual ~SpotEmitter() = default;

    virtual SpectrumRGB evaluate(const EmitterQueryRecord &eRec) const override
    {
        //* No implement
        std::cout << "No implement!\n";
        std::exit(1);
    }

    virtual SpectrumRGB evaluate(const Ray3f &ray) const override
    {
        //* No implement
        std::cout << "No implement!\n";
        std::exit(1);
    }

    virtual void sample(PointQueryRecord*, Point2f) const override
    {
        //todo delete this
    }

    virtual void sample(DirectIlluminationRecord *dRec, 
                        Point3f sample,
                        Point3f from) const override
    {
        dRec->emitter_type = DirectIlluminationRecord::EmitterType::ESpot;
        dRec->isDelta = true;
        dRec->pdf = 1;
        dRec->shadow_ray = Ray3f{from, position};
        dRec->energy = lightEnergy / (dRec->shadow_ray.tmax * dRec->shadow_ray.tmax);
    }

    virtual std::pair<Point3f, float> samplePoint(Point3f sample) const override {
        return {position, 1};
    }

    //todo delete this
    virtual void setTexture(Texture *envmap) override{ }

    virtual float pdf(const EmitterHitInfo &info) const override{
        return 1;
    }
protected:
    Point3f position;
    SpectrumRGB lightEnergy;

};
REGISTER_CLASS(SpotEmitter, "spot")