#include "scene.h"

#include <stack>

#include "core/mesh/meshSet.h"
#include "emitter.h"
#include "medium.h"
#include "sampler.h"
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
  //* old
  // RayIntersectionRec iRec;
  // return accelPtr->rayIntersect(ray, iRec);
  return accelPtr->rayIntersect(ray);
}

bool Scene::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
  return accelPtr->rayIntersect(ray, iRec);
}

// TODO, when emitter and envmap both exist, solve the problem
void Scene::sampleEmitterOnSurface(PointQueryRecord &pRec,
                                   Sampler *sampler) const {
  if (emitterList.size() != 0) {
    // TODO, pdf should add to sample & sampleOnSurface function
    size_t emitterIdx = emitterDistribution.sample(sampler->next1D());
    emitterList[emitterIdx]->sampleOnSurface(pRec, sampler);
    pRec.pdf = 1.f / emitterSurfaceArea;
  } else {
    //! No area emitter in scene, sample the environment
    m_env_emitter->sample(&pRec, sampler->next2D());
  }
}

void Scene::setEnvMap(Emitter *env_emitter) {
  // assert(env_emitter != nullptr);
  m_env_emitter = env_emitter;
}

SpectrumRGB Scene::evaluateEnvironment(const Ray3f &ray) const {
  if (m_env_emitter == nullptr) {
    return SpectrumRGB{.0f};
  } else {
    return m_env_emitter->evaluate(ray);
  }
}

float Scene::pdfEnvironment(const Ray3f &ray) const {
  return m_env_emitter->pdf(ray);
}

float Scene::pdfArea(const RayIntersectionRec &i_rec, const Ray3f &ray) const {
  return (1 / emitterSurfaceArea) * i_rec.t * i_rec.t /
         std::abs(dot(i_rec.geoN, ray.dir));
}

void Scene::sampleDirectIllumination(DirectIlluminationRecord *d_rec,
                                     Sampler *sampler, Point3f from) const {
  // TODO, just sample area light now
  if (!emitterList.empty()) {
    PointQueryRecord p_rec;
    size_t emitterIdx = emitterDistribution.sample(sampler->next1D());
    emitterList[emitterIdx]->sampleOnSurface(p_rec, sampler);
    p_rec.emitter = emitterList[emitterIdx]->getEmitter();
    Ray3f shadow_ray{from, p_rec.p};
    EmitterQueryRecord e_rec{p_rec, shadow_ray};

    d_rec->point_on_emitter = p_rec.p;
    d_rec->shadow_ray = shadow_ray;
    d_rec->emitter_type = DirectIlluminationRecord::EmitterType::EArea;
    d_rec->energy = p_rec.emitter->evaluate(e_rec);
    d_rec->pdf = 1 / emitterSurfaceArea;
    d_rec->pdf *= shadow_ray.tmax * shadow_ray.tmax /
                  std::abs(dot(p_rec.normal, shadow_ray.dir));
  } else {
    std::cout << "No area light!\n";
    std::exit(1);
  }
}

void Scene::sampleAttenuatedDirectIllumination(
    DirectIlluminationRecord *d_rec, Sampler *sampler, Point3f from,
    SpectrumRGB *transmittance, const std::stack<Medium *> cur_stack) const {
  // TODO, just sample area light now
  if (!emitterList.empty()) {
    PointQueryRecord p_rec;
    size_t emitterIdx = emitterDistribution.sample(sampler->next1D());
    emitterList[emitterIdx]->sampleOnSurface(p_rec, sampler);
    p_rec.emitter = emitterList[emitterIdx]->getEmitter();
    Ray3f shadow_ray{from, p_rec.p};
    EmitterQueryRecord e_rec{p_rec, shadow_ray};

    d_rec->point_on_emitter = p_rec.p;
    d_rec->shadow_ray = shadow_ray;

    //* Shadowray intersect test
    //* 1. If occlude
    if (rayIntersect(shadow_ray)) {
      *transmittance = SpectrumRGB{.0f};
      return;
    } else {
      //* 2. Evaluate the possible transmittance
      std::stack<Medium *> mediums = cur_stack;
      RayIntersectionRec i_rec;
      Ray3f transimattance_ray = shadow_ray;
      *transmittance = SpectrumRGB{1.0f};
      while (rayIntersect(transimattance_ray, i_rec)) {
        //* If in volume
        if (!mediums.empty()) {
          Medium *medium = mediums.top();
          *transmittance *=
              medium->transmittance(transimattance_ray.ori, i_rec.p);
        }
        //* If enter the volume
        if (dot(i_rec.geoN, transimattance_ray.dir) < 0) {
          if (i_rec.meshPtr->getMedium())
            mediums.push(i_rec.meshPtr->getMedium());
        }
        //* If escape the volume
        if (dot(i_rec.geoN, transimattance_ray.dir) > 0) {
          if (i_rec.meshPtr->getMedium() && !mediums.empty()) mediums.pop();
        }
        //* Update transimattance ray
        transimattance_ray = Ray3f{i_rec.p, d_rec->point_on_emitter};
        //* Clear the i_rec
        i_rec.clear();
      }
    }

    d_rec->emitter_type = DirectIlluminationRecord::EmitterType::EArea;
    d_rec->energy = p_rec.emitter->evaluate(e_rec);
    d_rec->pdf = 1 / emitterSurfaceArea;
    d_rec->pdf *= shadow_ray.tmax * shadow_ray.tmax /
                  std::abs(dot(p_rec.normal, shadow_ray.dir));
  } else {
    std::cout << "No area light!\n";
    std::exit(1);
  }
}

SpectrumRGB Scene::getTransmittance(const std::stack<Medium *> &medium_stack,
                                    Point3f from, Point3f to) const {
  Ray3f transimattance_ray{from, to};

  //* Shadowray intersect test
  //* 1. If occlude
  if (rayIntersect(transimattance_ray)) {
    return SpectrumRGB{.0f};
  } else {
    //* 2. Evaluate the possible return
    std::stack<Medium *> mediums = medium_stack;
    RayIntersectionRec i_rec;
    SpectrumRGB res{1.f};
    while (rayIntersect(transimattance_ray, i_rec)) {
      //* If in volume
      if (!mediums.empty()) {
        Medium *medium = mediums.top();
        res *= medium->transmittance(transimattance_ray.ori, i_rec.p);
      }
      //* If enter the volume
      if (dot(i_rec.geoN, transimattance_ray.dir) < 0) {
        if (i_rec.meshPtr->getMedium())
          mediums.push(i_rec.meshPtr->getMedium());
      }
      //* If escape the volume
      if (dot(i_rec.geoN, transimattance_ray.dir) > 0) {
        if (i_rec.meshPtr->getMedium() && !mediums.empty()) mediums.pop();
      }
      //* Update transimattance ray
      transimattance_ray = Ray3f{i_rec.p, to};
      //* Clear the i_rec
      i_rec.clear();
    }
    return res;
  }
}
