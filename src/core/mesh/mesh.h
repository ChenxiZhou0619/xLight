#pragma once
#include <memory>
#include <vector>

#include <assimp/scene.h>

#include "core/geometry/geometry.h"

class Mesh {
public:
    Mesh() = default;

    virtual ~Mesh() = default;

    Mesh(const Mesh&) = delete;

    Mesh& operator=(const Mesh&) = delete;

    virtual std::string toString() const = 0;
};

class TriMesh : public Mesh{
public:

    TriMesh() = default;

    TriMesh(const TriMesh&) = delete;

    TriMesh& operator=(const TriMesh&) = delete;
    
    TriMesh(
        std::vector<Point3f> &&_mVerticesBuf,
        std::vector<Normal3f> &&_mNormalsBuf,
        std::vector<Point3ui> &&_mFacesBuf,
        std::vector<Point2f> &&_mUVsBuf
    ): mVerticesBuf(std::make_unique<std::vector<Point3f>>(_mVerticesBuf)),
       mNormalsBuf(std::make_unique<std::vector<Normal3f>>(_mNormalsBuf)),
       mFacesBuf(std::make_unique<std::vector<Point3ui>>(_mFacesBuf)),
       mUVsBuf(std::make_unique<std::vector<Point2f>>(_mUVsBuf)) {}

    virtual ~TriMesh() = default;


    virtual std::string toString() const;

protected:

    std::unique_ptr<std::vector<Point3f>>  mVerticesBuf;   // vertices
    std::unique_ptr<std::vector<Normal3f>> mNormalsBuf;    // normals per vertex
    std::unique_ptr<std::vector<Point3ui>> mFacesBuf;      // mesh faces
    std::unique_ptr<std::vector<Point2f>>  mUVsBuf;        // uv coordinates
};