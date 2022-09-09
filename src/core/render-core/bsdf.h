#pragma once
#include "core/utils/configurable.h"
#include "spectrum.h"
#include "texture.h"
#include "core/shape/shape.h"

struct BSDFQueryRecord {
    // ! both point from the origin in local
    Vector3f wi, wo;
    Point2f uv;
    float du, dv;

    BSDFQueryRecord() = default;
    BSDFQueryRecord(const Vector3f &_wi) : wi(_wi) { }
    BSDFQueryRecord(const Vector3f &_wi, Point2f _uv) : wi(_wi), uv(_uv) { }
    BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo) : wi(_wi), wo(_wo) { }
    BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo, Point2f _uv) : wi(_wi), wo(_wo), uv(_uv) { }

    BSDFQueryRecord(const RayIntersectionRec &i_rec, const Ray3f &ri) {
        wi = i_rec.toLocal(-ri.dir);
        uv = i_rec.UV;
        if (ri.is_ray_differential && i_rec.can_diff) {
            du = i_rec.dudx + i_rec.dudy;
            dv = i_rec.dvdx + i_rec.dvdy;
        } else {
            du = dv = .0f;
        }
    }

    BSDFQueryRecord(const RayIntersectionRec &i_rec, const Ray3f &ri, const Ray3f &ro) {
        wi = i_rec.toLocal(-ri.dir);
        wo = i_rec.toLocal(ro.dir);
        uv = i_rec.UV;
        if (ri.is_ray_differential && i_rec.can_diff) {
            du = i_rec.dudx + i_rec.dudy;
            dv = i_rec.dvdx + i_rec.dvdy;
        } else {
            du = dv = .0f;
        }
    }

    BSDFQueryRecord(const ShapeIntersection &its, Ray3f ri, Ray3f ro) {
        wi = its.toLocal(-ri.dir);
        wo = its.toLocal(ro.dir);
        uv = its.uv;
        du = dv = 0;
    }
    BSDFQueryRecord(const ShapeIntersection &its, Ray3f ri) {
        wi = its.toLocal(-ri.dir);
        uv = its.uv;
        du = dv = 0;
    }
};

class BSDF : public Configurable{
protected:
    std::shared_ptr<Texture> m_texture;
    Texture *m_bumpmap;
    Texture *m_normalmap;
public:
    BSDF() = default;
    BSDF(const rapidjson::Value &_value);
    virtual ~BSDF() = default;

    //TODO delete it
    void setTexture(Texture* texture) {
        m_texture.reset(texture);
    }

    void setTexture(std::shared_ptr<Texture> texture) {
        m_texture = texture;
    }

    void setBumpmap(Texture *texture) {
        m_bumpmap = texture;
    }

    void setNormalmap(Texture *texture) {
        m_normalmap = texture;
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

    virtual void bumpComputeShadingNormal(RayIntersectionRec *i_rec) const;

    void computeShadingNormal (RayIntersectionRec *i_rec) const;

    virtual bool isDiffuse() const = 0;

    enum class EBSDFType {
        EEmpty = 0,
        EGlossy,
        EDiffuse
    } m_type;
};