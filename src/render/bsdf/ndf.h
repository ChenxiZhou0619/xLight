#pragma once
#include "core/geometry/geometry.h"

//* Normal distribution function
class NDF {
public:
    virtual float eval(Normal3f n) const = 0;

    virtual std::pair<Normal3f, float> sample(Vector3f wi, Point2f sample) const = 0;

    virtual float pdf(Vector3f wi, Vector3f n) const = 0;

    float G(Vector3f wi, Vector3f wo) const {
        return 1 / (1 + Lambda(wi)) / (1 + Lambda(wo));
    }

    virtual float Lambda(Vector3f v) const = 0;

};

class GGX : public NDF {
public:
    GGX() = default;

    GGX(float roughness, float anisotropic)
        : mRoughness(roughness), mAnisotropic(anisotropic) 
    {   
        float aspect = std::sqrt(1 - 0.9 * mAnisotropic);
        mAlphaX = std::max(mAlphaMin, mRoughness * mRoughness / aspect);
        mAlphaY = std::max(mAlphaMin, mRoughness * mRoughness * aspect);
    }

    virtual float eval(Normal3f n) const override
    {
        float determinant1 = (M_PI * mAlphaX * mAlphaY);
        float determinant2 = 
            std::pow(n.x / mAlphaX, 2) +
            std::pow(n.z / mAlphaY, 2) + 
            n.y * n.y;
        return 1 / (determinant1 * determinant2 * determinant2); 
    }

    virtual std::pair<Normal3f, float> sample(Vector3f wi, Point2f sample) const override
    {
        Vector3f Vh = normalize(Vector3f(wi.x * mAlphaX, wi.y, wi.z * mAlphaY));
        float lensq = Vh.x * Vh.x + Vh.z * Vh.z;
        Vector3f T1 = lensq > 0 ? Vector3f(-Vh.z, Vh.x, 0) / std::sqrt(lensq) : Vector3f(1, 0, 0);
        Vector3f T2 = cross(Vh, T1);

        float r = std::sqrt(sample.x);
        float phi = 2 * M_PI * sample.y;
        float t1 = r * std::cos(phi),
              t2 = r * std::sin(phi);
        float s = 0.5 * (1 + Vh.y);
        t2 = (1.0 - s) * std::sqrt(1.0 - t1 * t1) + s * t2;
        Vector3f Nh = t1 * T1 + t2 * T2 + std::sqrt(std::max(0.f, 1 - t1 * t1 - t2 * t2)) * Vh;
        Vector3f Ne = normalize(Vector3f(mAlphaX * Nh.x, std::max(.0f, Nh.y), mAlphaY * Nh.z));
        float pdf = 1 / (1 + Lambda(wi)) * std::max(0.f, dot(wi, Ne)) * eval(Ne) / wi.y;
        float term1 = 1 / (1 + Lambda(wi)),
              term2 = std::max(0.f, dot(wi, Ne)),
              term3 = eval(Ne);
        return {Ne, pdf};
    }

    virtual float pdf(Vector3f wi, Vector3f n) const override 
    {
        float pdf = 1 / (1 + Lambda(wi)) * std::max(0.f, dot(wi, n)) * eval(n) / wi.y;
        return pdf;
    }

    virtual float Lambda(Vector3f v) const override {
        return (std::sqrt(1 + (std::pow(v.x * mAlphaX, 2) + std::pow(v.z * mAlphaY, 2)) / (v.y * v.y)) - 1) * 0.5f;
    }

protected:
    float mRoughness;
    float mAnisotropic;
    float mAlphaX, mAlphaY;
    const float mAlphaMin = 0.0001f;
};