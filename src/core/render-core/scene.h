#pragma once
#include "core/mesh/meshLoader.h"
#include "core/geometry/geometry.h"
#include "camera.h"

class Scene {
public:
    Scene() = default;

    Scene(std::string sceneFile) {
        MeshLoader loader;
        meshSetPtr = loader.loadFromFile(sceneFile);
    }

    void preprocess();

    bool rayIntersect(const Ray3f &ray) const ;

    bool rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const ;

private:
    std::unique_ptr<MeshSet> meshSetPtr;
};