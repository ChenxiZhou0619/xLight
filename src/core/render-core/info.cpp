#include <core/render-core/info.h>
#include <core/scene/scene.h>

//* IntersectionInfo
Vector3f IntersectionInfo::toLocal(Vector3f v) const {
    return shadingFrame.toLocal(v);
}
Vector3f IntersectionInfo::toWorld(Vector3f v) const {
    return shadingFrame.toWorld(v);
}

//* SurfaceIntersectionInfo
Ray3f SurfaceIntersectionInfo::scatterRay(const Scene &scene,
                                          Point3f destination) const
{
    Ray3f ray{position, destination};
    ray.ori += ray.dir * 0.0001;
    bool outwards = dot(ray.dir, geometryNormal) > 0;
    auto medium = outwards ? 
        scene.getEnvMedium() : shape->getInsideMedium();
    ray.medium = medium ? medium.get() : nullptr; 
    return ray;
}

Ray3f SurfaceIntersectionInfo::scatterRay(const Scene &scene,
                                          Vector3f direction) const
{
    Ray3f ray{position, direction};
    ray.ori += ray.dir * 0.0001;
    bool outwards = dot(ray.dir, geometryNormal) > 0;
    auto medium = outwards ? 
        scene.getEnvMedium() : shape->getInsideMedium();
    ray.medium = medium ? medium.get() : nullptr; 
    return ray;
}

SpectrumRGB SurfaceIntersectionInfo::evaluateScatter(Vector3f wo) const
{
    assert(shape);
    return shape->getBSDF()->evaluate(*this, wo);
}

float SurfaceIntersectionInfo::pdfScatter(Vector3f wo) const
{
    assert(shape);
    return shape->getBSDF()->pdf(*this, wo);
}

ScatterInfo SurfaceIntersectionInfo::sampleScatter(Point2f sample) const
{
    assert(shape);
    auto sInfo = shape->getBSDF()->sample(*this, sample);
    sInfo.scatterType = ScatterInfo::ScatterType::Surface;
    return sInfo;
}

SpectrumRGB SurfaceIntersectionInfo::evaluateLe() const
{
    return light ? 
        light->evaluate(*this) : SpectrumRGB{.0f};
}

float SurfaceIntersectionInfo::pdfLe() const
{
    return light ?
        light->pdf(*this) : .0f;
}

bool SurfaceIntersectionInfo::terminate() const 
{
    return (!shape || shape->isEmitter());
}

void SurfaceIntersectionInfo::computeShadingFrame() {
    if (!terminate())
        this->shape->getBSDF()->computeShadingFrame(this);
}

//* MediumIntersectionInfo
Ray3f MediumIntersectionInfo::scatterRay(const Scene &scene,
                                          Point3f destination) const
{
    Ray3f ray{position, destination};
    ray.medium = medium;
    return ray;
}

Ray3f MediumIntersectionInfo::scatterRay(const Scene &scene,
                                          Vector3f direction) const
{
    Ray3f ray{position, direction};
    ray.medium = medium;
    return ray;
}

SpectrumRGB MediumIntersectionInfo::evaluateScatter(Vector3f wo) const 
{
    assert(medium);
    PhaseQueryRecord pRec{position, wi, wo};
    return medium->evaluatePhase(pRec);
}

float MediumIntersectionInfo::pdfScatter(Vector3f wo) const
{
    assert(medium);
    PhaseQueryRecord pRec{position, wi, wo};
    return medium->pdfPhase(pRec);
}

ScatterInfo MediumIntersectionInfo::sampleScatter(Point2f sample) const
{
    assert(medium);
    ScatterInfo info;
    PhaseQueryRecord pRec{position, wi};
    info.weight = medium->samplePhase(&pRec, sample);
    info.wo = toWorld(pRec.wo);
    info.pdf = medium->pdfPhase(pRec);
    info.scatterType = ScatterInfo::ScatterType::Medium;
    return info;
}

SpectrumRGB MediumIntersectionInfo::evaluateLe() const
{
    //todo no emission medium now
    return SpectrumRGB{0};
}

float MediumIntersectionInfo::pdfLe() const
{
    //todo no emission medium now
    return .0f;
}

bool MediumIntersectionInfo::terminate() const 
{
    //todo no emission, so always false
    return false;
}

void MediumIntersectionInfo::computeShadingFrame() {
    //* do nothing
}