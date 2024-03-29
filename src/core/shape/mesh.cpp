#include "mesh.h"

#include "core/render-core/sampler.h"

std::unordered_map<std::string, std::shared_ptr<ShapeInterface>>
loadObjFile(const std::string &filePath) {
  std::unordered_map<std::string, std::shared_ptr<ShapeInterface>> result;

  Assimp::Importer importer;
  const aiScene *ai_scene =
      importer.ReadFile(filePath, // aiProcess_ConvertToLeftHanded |
                        aiProcess_JoinIdenticalVertices |
                            // aiProcess_CalcTangentSpace |
                            aiProcess_Triangulate);

  if (ai_scene == nullptr) {
    std::cerr << "Error: Parsing obj file!\n";
    std::exit(1);
  }

  for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
    auto ai_mesh = ai_scene->mMeshes[i];
    std::cout << "Mesh : " << ai_mesh->mName.C_Str() << std::endl;
    std::shared_ptr<TriangleMesh> triMesh = std::make_shared<TriangleMesh>();

    if (!ai_mesh->HasPositions()) {
      std::cerr << "Mesh with no vertexes!\n";
      std::exit(1);
    }
    //*---------- Parsing vertexes ----------
    int numVertexes = ai_mesh->mNumVertices;
    triMesh->m_vertexes.resize(3, numVertexes);
    std::memcpy(triMesh->m_vertexes.data(), ai_mesh->mVertices,
                numVertexes * sizeof(aiVector3D));

    if (!ai_mesh->HasFaces()) {
      std::cerr << "Mesh with no faces!\n";
      std::exit(1);
    }
    //*---------- Parsing faces -------------
    int numFaces = ai_mesh->mNumFaces;
    triMesh->m_faces.reserve(numFaces);
    triMesh->m_triangles_distribution =
        std::make_shared<Distribution1D>(numFaces);
    for (int j = 0; j < ai_mesh->mNumFaces; ++j) {
      auto face = ai_mesh->mFaces[j].mIndices;
      triMesh->m_faces.emplace_back(Point3ui(face[0], face[1], face[2]));
    }

    if (!ai_mesh->HasNormals()) {
      std::cerr << "Mesh with no normals!\n";
      std::exit(1);
    }
    //*--------- Parsing normals -----------
    triMesh->m_normals.resize(3, numVertexes);
    std::memcpy(triMesh->m_normals.data(), ai_mesh->mNormals,
                numVertexes * sizeof(aiVector3D));

    //*---------- Parsing tangents ---------
    if (!ai_mesh->HasTangentsAndBitangents())
      triMesh->hasTangent = false;
    else {
      triMesh->hasTangent = true;
      triMesh->m_tangents.resize(3, numVertexes);
      std::memcpy(triMesh->m_tangents.data(), ai_mesh->mTangents,
                  numVertexes * sizeof(aiVector3D));
    }

    //*----------- Parsing UVs -------------
    if (ai_mesh->GetNumUVChannels() == 0)
      triMesh->hasUV = false;
    else {
      triMesh->hasUV = true;
      triMesh->m_UVs.reserve(numVertexes);
      auto &uv = ai_mesh->mTextureCoords[0];
      for (int j = 0; j < numVertexes; ++j) {
        triMesh->m_UVs.emplace_back(Point2f(uv[j][0], uv[j][1]));
      }
    }
    result[ai_mesh->mName.C_Str()] = triMesh;
  }
  return result;
}

void TriangleMesh::initEmbreeGeometry(RTCDevice device) {
  this->embreeGeometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

  float *vertexes = (float *)rtcSetNewGeometryBuffer(
      this->embreeGeometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
      3 * sizeof(float), m_vertexes.cols());

  // brute-force copy
  for (int i = 0; i < m_vertexes.cols(); ++i) {
    vertexes[i * 3 + 0] = m_vertexes.col(i).x();
    vertexes[i * 3 + 1] = m_vertexes.col(i).y();
    vertexes[i * 3 + 2] = m_vertexes.col(i).z();
  }

  unsigned *faces = (unsigned *)rtcSetNewGeometryBuffer(
      this->embreeGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
      3 * sizeof(unsigned), m_faces.size());

  // brute-force copy
  //* compute the triangle distribution
  for (int i = 0; i < m_faces.size(); ++i) {
    faces[i * 3 + 0] = m_faces[i].x;
    faces[i * 3 + 1] = m_faces[i].y;
    faces[i * 3 + 2] = m_faces[i].z;

    m_triangles_distribution->append(getTriArea(i));
  }
  m_surface_area = m_triangles_distribution->normalize();

  rtcCommitGeometry(this->embreeGeometry);
}

Point3f TriangleMesh::getVertex(int idx) const {
  auto vertex = m_vertexes.col(idx);
  return Point3f{vertex.x(), vertex.y(), vertex.z()};
}

Point3ui TriangleMesh::getFace(int idx) const { return m_faces[idx]; }

Normal3f TriangleMesh::getNormal(int idx) const {
  auto normal = m_normals.col(idx);
  return Normal3f{normal.x(), normal.y(), normal.z()};
}

Point2f TriangleMesh::getUV(int idx) const { return m_UVs[idx]; }

float TriangleMesh::getTriArea(int idx) const {
  auto triangle = this->getFace(idx);
  auto p0 = this->getVertex(triangle.x), p1 = this->getVertex(triangle.y),
       p2 = this->getVertex(triangle.z);
  return 0.5 * cross((p0 - p1), (p0 - p2)).length();
}

Point3f TriangleMesh::getHitPoint(int triIdx, Point2f uv) const {
  auto [u, v] = uv;
  auto triangle = this->getFace(triIdx);
  auto p0 = this->getVertex(triangle.x), p1 = this->getVertex(triangle.y),
       p2 = this->getVertex(triangle.z);
  return (1 - u - v) * p0 + u * p1 + v * p2;
}

Normal3f TriangleMesh::getHitNormal(int triIdx, Point2f uv) const {
  auto [u, v] = uv;
  auto triangle = this->getFace(triIdx);
  auto n0 = this->getNormal(triangle.x), n1 = this->getNormal(triangle.y),
       n2 = this->getNormal(triangle.z);
  return (1 - u - v) * n0 + u * n1 + v * n2;
}

Vector3f TriangleMesh::getTangent(int idx) const {
  auto tangent = m_tangents.col(idx);
  return Vector3f{tangent.x(), tangent.y(), tangent.z()};
}

Vector3f TriangleMesh::getHitTangent(int triIdx, Point2f uv) const {
  auto [u, v] = uv;
  auto triangle = this->getFace(triIdx);
  auto t0 = this->getTangent(triangle.x), t1 = this->getTangent(triangle.y),
       t2 = this->getTangent(triangle.z);
  return (1 - u - v) * t0 + u * t1 + v * t2;
}

Vector3f TriangleMesh::dpdu(int triIdx) const {
  auto triangle = getFace(triIdx);
  auto uv0 = getUV(triangle.x), uv1 = getUV(triangle.y),
       uv2 = getUV(triangle.z);

  auto p0 = getVertex(triangle.x), p1 = getVertex(triangle.y),
       p2 = getVertex(triangle.z);

  float determinant = (uv0[0] - uv2[0]) * (uv1[1] - uv2[1]) -
                      (uv0[1] - uv2[1]) * (uv1[0] - uv2[0]);

  Vector3f dpdu = (uv1[1] - uv2[1]) * (p0 - p2) + (uv2[1] - uv0[1]) * (p1 - p2);

  if (determinant != 0)
    dpdu /= determinant;
  return dpdu;
}

Vector3f TriangleMesh::dpdv(int triIdx) const {
  // do nothing currnetly
  std::exit(1);
}

Normal3f TriangleMesh::getHitNormal(int triIdx) const {
  auto triangle = this->getFace(triIdx);
  auto p0 = this->getVertex(triangle.x), p1 = this->getVertex(triangle.y),
       p2 = this->getVertex(triangle.z);
  return cross(p1 - p0, p2 - p0);
}

Point2f TriangleMesh::getHitTextureCoordinate(int triIdx, Point2f uv) const {
  if (!this->hasUV)
    return Point2f(.0f);
  auto [u, v] = uv;
  auto triangle = this->getFace(triIdx);
  auto uv0 = this->getUV(triangle.x), uv1 = this->getUV(triangle.y),
       uv2 = this->getUV(triangle.z);
  return (1 - u - v) * uv0 + u * uv1 + v * uv2;
}

void TriangleMesh::sampleOnSurface(PointQueryRecord *pRec,
                                   Point3f sample) const {
  int triIdx = m_triangles_distribution->sample(sample[0]);
  auto triangle = getFace(triIdx);
  auto p0 = this->getVertex(triangle.x), p1 = this->getVertex(triangle.y),
       p2 = this->getVertex(triangle.z);
  float x = sample[1], y = sample[2];
  auto w0 = 1 - std::sqrt(1 - x), w1 = y * std::sqrt(1 - x),
       w2 = std::max(.0f, 1 - w0 - w1);

  pRec->p = p0 * w0 + p1 * w1 + p2 * w2;
  pRec->normal = getNormal(triangle.x) * w0 + getNormal(triangle.y) * w1 +
                 getNormal(triangle.z) * w2;
  pRec->pdf = 1 / m_surface_area;
}

std::pair<Vector3f, Vector3f>
TriangleMesh::positionDifferential(int triIdx) const {
  //* Return dpdu & dpdv
  /*
   * | u01 v01 || dpdu |   | p01 |
   * |         ||      | = |     |
   * | u02 v02 || dpdv |   | p02 |
   */
  auto [i0, i1, i2] = this->getFace(triIdx);
  auto p0 = this->getVertex(i0), p1 = this->getVertex(i1),
       p2 = this->getVertex(i2);
  Point2f uv0 = getUV(i0), uv1 = getUV(i1), uv2 = getUV(i2);
  auto [u01, v01] = uv1 - uv0;
  auto [u02, v02] = uv2 - uv0;
  auto p01 = p1 - p0, p02 = p2 - p0;
  float determinant = u01 * v02 - v01 * u02;
  bool degenerate = std::abs(determinant) < 1e-8;
  Vector3f dpdu, dpdv;
  if (!degenerate) {
    float invDeterminant = 1.f / determinant;
    dpdu = (v02 * p01 - v01 * p02) * invDeterminant;
    dpdv = (-u02 * p01 + u01 * p02) * invDeterminant;
  }
  if (degenerate || cross(dpdu, dpdv).length2() == 0) {
    Normal3f n = cross(p1 - p0, p2 - p0);
    Coordinate(n, &dpdu, &dpdv);
  }
  return {dpdu, dpdv};
}