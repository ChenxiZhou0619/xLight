#pragma once
#include "core/utils/configurable.h"
#include "spectrum.h"
#include "texture.h"

class BSDF : public Configurable{
protected:
    Texture *m_texture;
public:
    BSDF() = default;
    BSDF(const rapidjson::Value &_value);
    ~BSDF() = default;

    void setTexture(Texture *texture) {
        m_texture = texture;
    }


    /**
     * @brief return the bsdf value * cosTheta
     * 
     * @param iRec 
     * @return SpectrumRGB 
     */
    virtual SpectrumRGB evaluate (const RayIntersectionRec &iRec) const = 0;

    /**
     * @brief return the pdf of given wo
     * 
     * @param iRec 
     * @return float 
     */
    virtual float pdf (const RayIntersectionRec &iRec) const = 0;

    /**
     * @brief sample a wo, return the bsdf value * cosTheta
     * 
     * @param iRec 
     * @return SpectrumRGB 
     */
    virtual SpectrumRGB sample(RayIntersectionRec &iRec, const Point2f &sample) const = 0;
};