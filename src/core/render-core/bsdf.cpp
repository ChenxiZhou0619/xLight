#include "bsdf.h"

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