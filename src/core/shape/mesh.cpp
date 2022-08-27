#include "mesh.h"

std::vector<std::shared_ptr<ShapeInterface>>
loadObjFile(const std::string &filePath) {
    std::vector<std::shared_ptr<ShapeInterface>> result;

    Assimp::Importer importer;
    const aiScene *ai_scene = importer.ReadFile(
        filePath,
        aiProcess_ConvertToLeftHanded |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace | 
        aiProcess_Triangulate
    );

    if (ai_scene == nullptr) {
        std::cerr << "Error: Parsing obj file!\n";
        std::exit(1);
    }

    for (int i = 0; i < ai_scene->mNumMeshes; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        std::shared_ptr<TriangleMesh> triMesh = std::make_shared<TriangleMesh>();
        
        if(!ai_mesh->HasPositions()) {
            std::cerr << "Mesh with no vertexes!\n";
            std::exit(1);
        }
        //*---------- Parsing vertexes ----------
        int numVertexes = ai_mesh->mNumVertices;
        triMesh->m_vertexes.resize(3, numVertexes);
        std::memcpy(
            triMesh->m_vertexes.data(),
            ai_mesh->mVertices,
            numVertexes * sizeof(aiVector3D)
        );

        if(!ai_mesh->HasFaces()) {
            std::cerr << "Mesh with no faces!\n";
            std::exit(1);
        }
        //*---------- Parsing faces -------------
        int numFaces = ai_mesh->mNumFaces;
        triMesh->m_faces.reserve(numFaces);
        for (int j = 0; j < ai_mesh->mNumFaces; ++j) {
            auto face = ai_mesh->mFaces[j].mIndices;
            triMesh->m_faces.emplace_back(
                Point3ui(
                    face[0], face[1], face[2]
                )
            );
        }

        if (!ai_mesh->HasNormals()) {
            std::cerr << "Mesh with no normals!\n";
            std::exit(1);
        }
        //*--------- Parsing normals -----------
        triMesh->m_normals.resize(3, numVertexes);
        std::memcpy(
            triMesh->m_normals.data(), 
            ai_mesh->mNormals, 
            numVertexes * sizeof(aiVector3D)
        );

        //*---------- Parsing tangents ---------
        if (!ai_mesh->HasTangentsAndBitangents())
            triMesh->hasTangent = false;
        else {
            triMesh->hasTangent = true;
            triMesh->m_tangents.resize(3, numVertexes);
            std::memcpy(
                triMesh->m_tangents.data(), 
                ai_mesh->mTangents, 
                numVertexes * sizeof(aiVector3D)
            );
        }

        //*----------- Parsing UVs -------------
        if (ai_mesh->GetNumUVChannels() == 0) 
            triMesh->hasUV = false;
        else {
            triMesh->hasUV = true;
            triMesh->m_UVs.reserve(numVertexes);
            auto &uv = ai_mesh->mTextureCoords[0];
            for (int j = 0; j < numVertexes; ++j) {
                triMesh->m_UVs.emplace_back(
                    Point2f(uv[j][0], uv[j][1])
                );
            }
        }
        result.emplace_back(triMesh);
    }
    return result;
}

void TriangleMesh::initEmbreeGeometry(RTCDevice device) {
    this->embreeGeometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    float *vertexes = (float *) rtcSetNewGeometryBuffer(
        this->embreeGeometry, 
        RTC_BUFFER_TYPE_VERTEX, 
        0, 
        RTC_FORMAT_FLOAT3, 
        3 * sizeof(float), 
        m_vertexes.cols()  
    );

    // brute-force copy
    for (int i = 0; i < m_vertexes.cols(); ++i) {
        vertexes[i * 3 + 0] = m_vertexes.col(i).x();
        vertexes[i * 3 + 1] = m_vertexes.col(i).y();
        vertexes[i * 3 + 2] = m_vertexes.col(i).z();
    }
    
    unsigned *faces = (unsigned *) rtcSetNewGeometryBuffer(
        this->embreeGeometry, 
        RTC_BUFFER_TYPE_INDEX, 
        0, 
        RTC_FORMAT_UINT3, 
        3 * sizeof(unsigned),
        m_faces.size()
    );

    // brute-force copy
    for(int i = 0; i < m_faces.size(); ++i) {
        faces[i * 3 + 0] = m_faces[i].x;
        faces[i * 3 + 1] = m_faces[i].y;
        faces[i * 3 + 2] = m_faces[i].z;
    }

    rtcCommitGeometry(this->embreeGeometry);   
}

Point3f TriangleMesh::getVertex(int idx) const {
    auto vertex = m_vertexes.col(idx);
    return Point3f {
        vertex.x(), vertex.y(), vertex.z()
    };
}

Point3ui TriangleMesh::getFace(int idx) const {
    return m_faces[idx];
}

Normal3f TriangleMesh::getNormal(int idx) const {
    auto normal = m_normals.col(idx);
    return Normal3f {
        normal.x(), normal.y(), normal.z()
    };
}

Point2f TriangleMesh::getUV(int idx) const {
    return m_UVs[idx];
}

Point3f TriangleMesh::getHitPoint(int triIdx, Point2f uv) const {
    auto [u, v] = uv;
    auto triangle = this->getFace(triIdx);
    auto p0 = this->getVertex(triangle.x),
         p1 = this->getVertex(triangle.y),
         p2 = this->getVertex(triangle.z);
    return (1 - u - v) * p0 + u * p1 + v * p2;
}

Normal3f TriangleMesh::getHitNormal(int triIdx, Point2f uv) const {
    auto [u, v] = uv;
    auto triangle = this->getFace(triIdx);
    auto n0 = this->getNormal(triangle.x),
         n1 = this->getNormal(triangle.y),
         n2 = this->getNormal(triangle.z);
    return (1 - u - v) * n0 + u * n1 + v * n2;
}

Point2f TriangleMesh::getHitTextureCoordinate(int triIdx, Point2f uv) const {
    if (!this->hasUV)
        return Point2f(.0f);
    auto [u, v] = uv;
    auto triangle = this->getFace(triIdx);
    auto uv0 = this->getUV(triangle.x),
         uv1 = this->getUV(triangle.y),
         uv2 = this->getUV(triangle.z);
    return (1 - u - v) * uv0 + u * uv1 + v * uv2;
}