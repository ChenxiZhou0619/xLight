#pragma once
#include <memory>
#include <vector>
#include <optional>

#include <embree3/rtcore.h>
#include <core/shape/shape.h>

class Scene {
public:
    Scene();

    ~Scene() = default;

    void addShape(std::shared_ptr<ShapeInterface> mesh);

    void postProcess();

    std::optional<ShapeIntersection> intersect(const Ray3f &ray) const;


private:
    //* Embree 
    RTCDevice device;
    RTCScene scene;

    int shapeCount = 0;
    std::vector<std::shared_ptr<ShapeInterface>>  shapes;
};