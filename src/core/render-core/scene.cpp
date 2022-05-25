#include "scene.h"
#include "core/mesh/meshSet.h" 


/**
 * @brief build the accelaration structure
 * 
 */

void Scene::preprocess() {
    accelPtr->init();
}

bool Scene::rayIntersect(const Ray3f &ray) const {
    RayIntersectionRec iRec;
    return accelPtr->rayIntersect(ray, iRec);
}

bool Scene::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
    return accelPtr->rayIntersect(ray, iRec);
}