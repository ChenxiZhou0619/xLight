#pragma once
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <eigen3/Eigen/Eigen>
#include <vector>

#include "core/geometry/geometry.h"
#include "shape.h"

std::unordered_map<std::string, std::shared_ptr<ShapeInterface>> loadObjFile(
    const std::string &filePath);

class TriangleMesh : public ShapeInterface {
 public:
  TriangleMesh() = default;

  virtual ~TriangleMesh() = default;

  virtual void initEmbreeGeometry(RTCDevice device) override;

  //* Only for mesh like shape
  virtual Point3f getHitPoint(int triIdx, Point2f uv) const override;

  //* Only for mesh like shape
  virtual Normal3f getHitNormal(int triIdx, Point2f uv) const override;

  virtual Vector3f getHitTangent(int triIdx, Point2f uv) const override;

  virtual Vector3f dpdu(int triIdx) const override;

  virtual Vector3f dpdv(int triIdx) const override;

  virtual Normal3f getHitNormal(int triIdx) const override;

  //* Only for mesh like shape
  virtual Point2f getHitTextureCoordinate(int triIdx, Point2f) const override;

  virtual void sampleOnSurface(PointQueryRecord *pRec,
                               Point3f sample) const override;

  virtual std::pair<Vector3f, Vector3f> positionDifferential(
      int triIdx) const override;

 protected:
  virtual Point3f getVertex(int idx) const override;

  virtual Point3ui getFace(int idx) const override;

  virtual Normal3f getNormal(int idx) const override;

  virtual Point2f getUV(int idx) const override;

  Vector3f getTangent(int idx) const;

  float getTriArea(int idx) const;

 private:
  //* data
  Eigen::MatrixXf m_vertexes;
  Eigen::MatrixXf m_normals;
  Eigen::MatrixXf m_tangents;  // optional
  std::vector<Point3ui> m_faces;
  std::vector<Point2f> m_UVs;  // optional

  std::shared_ptr<Distribution1D> m_triangles_distribution;

  //* friend function
  friend std::unordered_map<std::string, std::shared_ptr<ShapeInterface>>
  loadObjFile(const std::string &);
};
