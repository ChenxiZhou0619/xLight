#include "scene.h"
#include "core/mesh/meshSet.h" 
#include "sampler.h"
#include "emitter.h"
/**
 * @brief build the accelaration structure
 * 
 */

void Scene::preprocess() {
    // construct the accelerate structure
    accelPtr->init();
    // construct the emitter distribution
    const auto &meshes = accelPtr->meshSetPtr->mMeshes;
    for (const auto &mesh : meshes) {
        if (mesh->isEmitter()) {
            // register the emitter
            emitterList.emplace_back(mesh.get());
            emitterDistribution.append(mesh->getMeshSurfaceArea());
        }
    }
    emitterSurfaceArea = emitterDistribution.normalize();
}

bool Scene::rayIntersect(const Ray3f &ray) const {
    RayIntersectionRec iRec;
    return accelPtr->rayIntersect(ray, iRec);
}

bool Scene::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
    return accelPtr->rayIntersect(ray, iRec);
}

void Scene::sampleEmitterOnSurface(PointQueryRecord &pRec, Sampler *sampler) const {
    size_t emitterIdx = emitterDistribution.sample(sampler->next1D());
    emitterList[emitterIdx]->sampleOnSurface(pRec, sampler);
    pRec.pdf = 1.f / emitterSurfaceArea;
}

void Scene::setEnvMap(Texture *_envmap) {
    envmap = _envmap;
}

SpectrumRGB Scene::evaluateEnvironment(const Ray3f &ray) const {
    if (envmap == nullptr) {
        return SpectrumRGB {.0f};
    } else {
        float cosTheta = ray.dir.y,
              tanPhi = ray.dir.z / ray.dir.x;
        float theta = std::acos(cosTheta),
              phi = std::atan(tanPhi);
        if (phi < 0) 
            phi += ray.dir.x > 0 ? 2 * M_PI : M_PI;
        else {
            phi += ray.dir.x > 0 ? .0f : M_PI;
        }
        return envmap->evaluate(Point2f(
            phi / (2 * M_PI),
            theta / M_PI
        ));
    }
}