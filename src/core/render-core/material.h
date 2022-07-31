#pragma once
#include "core/utils/configurable.h"
#include "bsdf.h"
#include "medium.h"
#include <optional>

class Material {
public:
    Material() = default;
    Material(const rapidjson::Value &_value) { }
    virtual ~Material() = default;

    virtual BSDF* getBSDF(const RayIntersectionRec &i_rec) const {
        return m_bsdf;
    }

    virtual Medium* getMedium(const RayIntersectionRec &i_rec) const {
        return m_medium;
    }

protected:
    Medium *m_medium;   //* The medium inside the mesh which the material is attached
    BSDF   *m_bsdf;     //* The bsdf attach to the mesh surface
};