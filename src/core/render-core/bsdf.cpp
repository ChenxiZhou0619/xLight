#include "bsdf.h"
#include <core/render-core/info.h>

void BSDF::bumpComputeShadingNormal(RayIntersectionRec *i_rec) const {
    if (m_bumpmap == nullptr)
        return;
    Normal3f n = i_rec->geoN;
    Point2f uv = i_rec->UV;

    float du = i_rec->dudx + i_rec->dudy,
          dv = i_rec->dvdx + i_rec->dvdy;

    float df_du = m_bumpmap->dfdu(i_rec->UV, du, dv)[0],
          df_dv = m_bumpmap->dfdv(i_rec->UV, du, dv)[0];

    i_rec->shdN = Normal3f (n + df_du * cross(n, i_rec->dpdv) - df_dv * cross(n, i_rec->dpdu));
    i_rec->shdFrame = Frame{i_rec->shdN};
}

void BSDF::computeShadingNormal(RayIntersectionRec *i_rec) const {
    if (m_normalmap == nullptr || !i_rec->hasTangent)
        return;
    
    Normal3f n = i_rec->geoN;
    Vector3f tangent = i_rec->tangent;
    Vector3f bitangent = cross(tangent, n);

    Frame TBN {n, tangent, bitangent};

    float du = 0, dv = 0;

    if (i_rec->can_diff) {
        du = i_rec->dudx + i_rec->dudy;
        dv = i_rec->dvdx + i_rec->dvdy;
    }

    auto normal_value = m_normalmap->evaluate(i_rec->UV, du, dv);
    
    //* Scale from [0, 1] to [-1, 1]
    Normal3f normal {
        normal_value.r() * 2 - 1,
        normal_value.b() * 2 - 1,
        normal_value.g() * 2 - 1
    };

    i_rec->shdN = TBN.toWorld(normal);
    i_rec->shdFrame = Frame(i_rec->shdN);
    
}

void BSDF::computeShadingFrame(ShapeIntersection *its) const 
{
    if (!m_normalmap) return;

    auto value = m_normalmap->evaluate(its->uv) * 2 - SpectrumRGB(1);
    //* cuz local frame using Y axis as up
    Normal3f shadingN = Normal3f{value.r(), value.b(), value.g()};
    shadingN = its->toWorld(shadingN);

    Vector3f newT = normalize(its->dpdu - shadingN * dot(shadingN, its->dpdu)),
             newB = cross(shadingN, newT);

    newT = cross(newB, shadingN);

    its->shadingF = Frame{shadingN, newT, newB};
}

SpectrumRGB BSDF::evaluate(const SurfaceIntersectionInfo &info,
                           Vector3f _wo) const 
{
    Vector3f wi = info.toLocal(info.wi),
             wo = info.toLocal(_wo);
    BSDFQueryRecord bRec;
    bRec.uv = info.uv;
    bRec.wi = wi;
    bRec.wo = wo;
    return evaluate(bRec);
}

BSDFInfo BSDF::sample(const SurfaceIntersectionInfo &info, 
                      Point2f uv) const
{
    BSDFInfo bsdfInfo;
    Vector3f wi = info.toLocal(info.wi);
    BSDFQueryRecord bRec;
    bRec.uv = info.uv;
    bRec.wi = wi;

    bsdfInfo.weight = sample(bRec, uv, bsdfInfo.pdf);
    bsdfInfo.wo = info.toWorld(bRec.wo);
    return bsdfInfo;
}

float BSDF::pdf(const SurfaceIntersectionInfo &info, 
                Vector3f _wo) const
{
    Vector3f wi = info.toLocal(info.wi),
             wo = info.toLocal(_wo);
    BSDFQueryRecord bRec;
    bRec.uv = info.uv;
    bRec.wi = wi;
    bRec.wo = wo;
    return pdf(bRec);
}