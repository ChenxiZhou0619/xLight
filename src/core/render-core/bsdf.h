#pragma once
#include "core/utils/configurable.h"
#include "spectrum.h"
#include "texture.h"

struct BSDFQueryRecord {
    // ! both point from the origin in local
    Vector3f wi, wo;
    Point2f uv;


    BSDFQueryRecord() = default;
    BSDFQueryRecord(const Vector3f &_wi) : wi(_wi) { }
    BSDFQueryRecord(const Vector3f &_wi, Point2f _uv) : wi(_wi), uv(_uv) { }
    BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo) : wi(_wi), wo(_wo) { }
    BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo, Point2f _uv) : wi(_wi), wo(_wo), uv(_uv) { }


};

class BSDF : public Configurable{
protected:
    Texture *m_texture;
public:
    BSDF() = default;
    BSDF(const rapidjson::Value &_value);
    virtual ~BSDF() = default;

    void setTexture(Texture *texture) {
        m_texture = texture;
    }

    virtual void initialize() {
        // do nothing
    }

    /**
     * @brief return the bsdf value * cosTheta
     * 
     * @param iRec 
     * @return SpectrumRGB 
     */
    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const = 0;

    /**
     * @brief return the pdf of given wo
     * 
     * @param iRec 
     * @return float 
     */
    virtual float pdf (const BSDFQueryRecord &bRec) const = 0;

    /**
     * @brief sample a wo, return the bsdf value * cosTheta / pdf
     * 
     * @param iRec 
     * @return SpectrumRGB 
     */
    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, float &pdf) const = 0;


    virtual bool isDiffuse() const = 0;
};