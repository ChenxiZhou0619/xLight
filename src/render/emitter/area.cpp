#include "core/render-core/emitter.h"
#include "core/shape/shape.h"
class AreaEmitter : public Emitter {
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
//todo depend the sampling strategy
    virtual float pdf(const EmitterHitInfo &info) const override 
    {
        if (auto shape_ptr = shape.lock(); 
            shape_ptr) 
        {
            float pdf = 1.f / shape_ptr->getSurfaceArea(),
                  jacob = info.dist * info.dist / std::abs(dot(info.normal, info.dir));
            return pdf * jacob;
        }   
        return .0f;             
    }

    virtual void sample(DirectIlluminationRecord *d_rec, 
                        Point3f sample, 
                        Point3f position) const override
    {
        auto shape_ptr = shape.lock();
        assert(shape_ptr != nullptr);

        PointQueryRecord pRec;
        shape_ptr->sampleOnSurface(&pRec, sample);
        Ray3f shadowRay {position, pRec.p};
        d_rec->emitter_type = DirectIlluminationRecord::EmitterType::EArea;
        d_rec->shadow_ray = shadowRay;
        d_rec->isDelta = false;
        d_rec->energy = evaluate(shadowRay);
        d_rec->pdf = shadowRay.tmax * shadowRay.tmax
            / std::abs(dot(pRec.normal, shadowRay.dir));
    }

    virtual std::pair<Point3f, float> samplePoint(Point3f sample) const override
    {
        auto shape_ptr = shape.lock();
        assert(shape_ptr != nullptr);

        PointQueryRecord pRec;
        shape_ptr->sampleOnSurface(&pRec, sample);
        return {pRec.p, pRec.pdf};
    }
};

REGISTER_CLASS(AreaEmitter, "area")