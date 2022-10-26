#include "core/render-core/emitter.h"
#include <core/render-core/info.h>

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

    virtual SpectrumRGB evaluate(const LightSourceInfo &info,
                                 Point3f destination) const override
    {
        return lightEnergy / (destination - info.position).length2();
    }

    virtual SpectrumRGB evaluate(const SurfaceIntersectionInfo &itsInfo,
                                 const Ray3f &ray) const override
    {
        //* No implement
        std::cout << "No implement!\n";
        std::exit(1);
    }

    virtual float pdf(const SurfaceIntersectionInfo &info, 
                      const Ray3f &ray) const override
    {
        return FINF;
    }

    virtual LightSourceInfo sampleLightSource(const SurfaceIntersectionInfo &info, 
                                              Point3f sample) const override
    {
        LightSourceInfo lightInfo;
        lightInfo.lightType = LightSourceInfo::LightType::Spot;
        lightInfo.light = this;
        lightInfo.direction = normalize(position - info.position);
        lightInfo.position = position;
        lightInfo.pdf = FINF;
        return lightInfo;
    }

protected:
    Point3f position;
    SpectrumRGB lightEnergy;

};
REGISTER_CLASS(SpotEmitter, "spot")