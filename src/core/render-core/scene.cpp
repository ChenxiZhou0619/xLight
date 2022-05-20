#include "scene.h"
#include "core/mesh/meshSet.h" 


/**
 * @brief build the accelaration structure
 * 
 */

void Scene::preprocess() {
    
}

bool Scene::rayIntersect(const Ray3f &ray) const {
    RayIntersectionRec iRec;
    return meshSetPtr->rayIntersect(ray, iRec);
}

bool Scene::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {
    return meshSetPtr->rayIntersect(ray, iRec);
}