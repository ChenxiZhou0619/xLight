#pragma once
#include <assimp/Importer.hpp>
#include <memory>
// forward decleration
class Mesh;


class MeshLoader {
public:
    MeshLoader() = default;

    std::unique_ptr<Mesh> loadMesh (const std::string &filePath) const ;
};