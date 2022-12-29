#include "task.h"

#include <fstream>
#include <functional>
#include <iostream>

#include "core/render-core/film.h"
#include "core/shape/gridmedium.h"
#include "core/shape/mesh.h"
#include "core/utils/configurable.h"
#include "rapidjson/document.h"

std::shared_ptr<Texture>
RenderTask::getTexture(const std::string &textureName) const {
  auto t_itr = textures.find(textureName);
  if (t_itr == textures.end()) {
    std::cerr << "No such a texture : \n" << textureName;
    std::exit(1);
  }
  return t_itr->second;
}

std::shared_ptr<BSDF> RenderTask::getBSDF(const std::string &bsdfName) const {
  auto b_itr = bsdfs.find(bsdfName);
  if (b_itr == bsdfs.end()) {
    std::cerr << "No such a BSDF : \n" << bsdfName;
    std::exit(1);
  }
  return b_itr->second;
}

std::shared_ptr<Emitter>
RenderTask::getEmitter(const std::string &emitterName) const {
  auto e_itr = emitters.find(emitterName);
  if (e_itr == emitters.end()) {
    std::cerr << "No such an emitter : \n" << emitterName;
    std::exit(1);
  }
  return e_itr->second;
}

std::shared_ptr<Medium>
RenderTask::getMedium(const std::string &mediumName) const {
  auto m_itr = mediums.find(mediumName);
  if (m_itr == mediums.end()) {
    std::cerr << "No such a medium : \n" << mediumName;
    std::exit(1);
  }
  return m_itr->second;
}

Point2i RenderTask::getImgSize() const { return film_size; }

int RenderTask::getSpp() const { return spp; }

void configureOutPut(std::shared_ptr<RenderTask> task,
                     const rapidjson::Value &config) {
  std::cout << "Configure output setting\n";
  const auto &output_name = config["filename"].GetString();
  const auto &output_size = config["size"].GetArray();
  //    task->image = std::make_shared<Image>(
  //        Vector2i{output_size[0].GetInt(), output_size[1].GetInt()},
  //        output_name,
  //        spp
  //    );
  task->spp = config["spp"].GetInt();
  task->file_name = output_name;
  task->film_size = Point2i{output_size[0].GetInt(), output_size[1].GetInt()};

  std::cout << "===== Output setting =====\n";
  std::cout << "filename : " << output_name << std::endl;
  std::cout << "spp : " << task->spp << std::endl;
}

void configureScene(std::shared_ptr<RenderTask> task,
                    const rapidjson::Value &config) {
  std::cout << "====== Configure scene =====\n";

  const auto &textures = config["textures"].GetArray();
  for (int i = 0; i < textures.Size(); ++i) {
    const auto &texture = textures[i].GetObject();
    const auto &textureName = texture["name"].GetString();
    const auto &textureType = texture["type"].GetString();
    std::shared_ptr<Texture> texture_ptr(static_cast<Texture *>(
        ObjectFactory::createInstance(textureType, texture)));
    task->textures[textureName] = texture_ptr;
  }

  std::cout << task->textures.size() << " textures configured\n";

  const auto &bsdfs = config["bsdfs"].GetArray();
  for (int i = 0; i < bsdfs.Size(); ++i) {
    const auto &bsdf = bsdfs[i].GetObject();
    const auto &bsdfName = bsdf["name"].GetString();
    const auto &bsdfType = bsdf["type"].GetString();
    std::shared_ptr<BSDF> bsdf_ptr{
        static_cast<BSDF *>(ObjectFactory::createInstance(bsdfType, bsdf))};

    if (bsdf.HasMember("textureRef")) {
      const auto &textureRef = bsdf["textureRef"].GetString();
      bsdf_ptr->setTexture(task->getTexture(textureRef));
    }
    if (bsdf.HasMember("normalmapRef")) {
      const auto &normalRef = bsdf["normalmapRef"].GetString();
      bsdf_ptr->setNormalmap(task->getTexture(normalRef));
    }
    // TODO add bumpmap and normalmap
    task->bsdfs[bsdfName] = bsdf_ptr;
    // TODO fix it
    bsdf_ptr->initialize();
  }

  std::cout << task->bsdfs.size() << " bsdfs configured\n";

  const auto &emitters = config["emitters"].GetArray();
  for (int i = 0; i < emitters.Size(); ++i) {
    const auto &emitter = emitters[i].GetObject();
    const auto &emitterType = emitter["type"].GetString();
    if (std::strcmp(emitterType, "envemitter") == 0) {
      const auto &textureRef = emitter["textureRef"].GetString();
      auto texture = task->getTexture(textureRef);
      // TODO set envmap
      std::shared_ptr<Emitter> envEmitter{static_cast<Emitter *>(
          ObjectFactory::createInstance("envemitter", emitter))};
      envEmitter->setTexture(texture.get());
      task->scene->setEnvMap(envEmitter);
      task->scene->addEmitter(envEmitter);
    } else if (std::strcmp("area", emitterType) == 0) {
      std::cout << "Area light should be declared directly in mesh!\n";
      std::exit(1);
    } else {
      //* Spot, directional etc...
      const auto &emitterName = emitter["name"].GetString();
      std::shared_ptr<Emitter> emitter_ptr{static_cast<Emitter *>(
          ObjectFactory::createInstance(emitterType, emitter))};
      task->scene->addEmitter(emitter_ptr);
    }
  }

  std::cout << task->emitters.size() << " emitters configured\n";

  const auto &mediums = config["mediums"].GetArray();
  for (int i = 0; i < mediums.Size(); ++i) {
    const auto &medium = mediums[i].GetObject();
    const auto &mediumType = medium["type"].GetString();
    const auto &mediumName = medium["name"].GetString();
    std::shared_ptr<Medium> medium_ptr{static_cast<Medium *>(
        ObjectFactory::createInstance(mediumType, medium))};

    if (medium.HasMember("phase")) {
      const auto &phaseType = medium["phase"]["type"].GetString();
      std::shared_ptr<PhaseFunction> phase{
          static_cast<PhaseFunction *>(ObjectFactory::createInstance(
              phaseType, medium["phase"].GetObject()))};
      medium_ptr->setPhase(phase);
    } else {
      std::shared_ptr<PhaseFunction> phase{static_cast<PhaseFunction *>(
          ObjectFactory::createInstance("isotropic", medium))};
      medium_ptr->setPhase(phase);
    }
    task->mediums[mediumName] = medium_ptr;

    if (medium.HasMember("isEnv")) {
      bool isEnv = medium["isEnv"].GetBool();
      if (isEnv) {
        task->scene->setEnvMedium(medium_ptr);
      }
    }
  }
  std::cout << task->mediums.size() << " mediums configured\n";

  const auto entities = config["entities"].GetArray();
  for (int i = 0; i < entities.Size(); ++i) {
    const auto &entity = entities[i].GetObject();
    const auto &filepath = entity["filepath"].GetString();
    const auto &fileType = entity["type"].GetString();
    if (std::strcmp("geometry-mesh", fileType) == 0) {
      const auto &meshes = loadObjFile(filepath);
      for (auto mesh : meshes) {
        task->scene->addShape(mesh.second);
      }
      const auto meshProperties = entity["meshProperties"].GetArray();
      for (int i = 0; i < meshProperties.Size(); ++i) {
        const auto &property = meshProperties[i].GetObject();
        const auto &meshName = property["meshName"].GetString();
        auto mesh = meshes.find(meshName);
        if (mesh == meshes.end()) {
          std::cerr << "No such a mesh : \"" << meshName << "\"\n";
          std::exit(1);
        }
        if (property.HasMember("emitter")) {
          auto emitter = property["emitter"].GetObject();
          auto emitterType = emitter["type"].GetString();

          std::shared_ptr<Emitter> emitter_ptr{static_cast<Emitter *>(
              ObjectFactory::createInstance(emitterType, emitter))};
          emitter_ptr->shape = mesh->second;
          mesh->second->setEmitter(emitter_ptr);
          mesh->second->setBSDF(std::make_shared<BlackHole>());
          task->scene->addEmitter(emitter_ptr);
        }
        if (property.HasMember("BSDFRef")) {
          const auto &bsdfName = property["BSDFRef"].GetString();
          auto bsdf = task->getBSDF(bsdfName);
          mesh->second->setBSDF(bsdf);
        }
        if (property.HasMember("mediumRef")) {
          const auto mediumName = property["mediumRef"].GetString();
          auto medium = task->getMedium(mediumName);
          mesh->second->setMedium(medium);
        }
        if (property.HasMember("twoSide")) {
          bool twoSide = property["twoSide"].GetBool();
          mesh->second->two_side = true;
        }
        if (property.HasMember("BSSRDF")) {
          bool bssrdf = property["BSSRDF"].GetBool();
          std::shared_ptr<BSSRDF> bssrdf_ptr{static_cast<BSSRDF *>(
              ObjectFactory::createInstance("seperate", property))};
          if (bssrdf)
            mesh->second->setBSSRDF(bssrdf_ptr);
        }
      }
    } else if (std::strcmp("grid-medium", fileType) == 0) {
      float scale = entity["scale"].GetFloat();
      const auto &grids = loadVdbFile(filepath, scale);
      for (auto grid : grids) {
        auto empty = task->getBSDF("empty_bsdf");
        grid.second->setBSDF(empty);
        task->scene->addShape(grid.second);
        std::shared_ptr<PhaseFunction> phase{static_cast<PhaseFunction *>(
            ObjectFactory::createInstance("isotropic", entity))};
        grid.second->getInsideMedium()->setPhase(phase);
      }
    }
  }
  task->scene->postProcess();
}

void configureRenderer(std::shared_ptr<RenderTask> task,
                       const rapidjson::Value &config) {
  std::cout << "====== Configure renderer =====\n";
  const auto &samplerType = config["sampler"]["type"].GetString();
  std::shared_ptr<Sampler> sampler_ptr{
      static_cast<Sampler *>(ObjectFactory::createInstance(
          samplerType, config["sampler"].GetObject()))};
  std::cout << "Sampler : " << samplerType << std::endl;
  task->sampler = sampler_ptr;

  const auto &cameraType = config["camera"]["type"].GetString();
  std::shared_ptr<Camera> camera_ptr{static_cast<Camera *>(
      ObjectFactory::createInstance(cameraType, config["camera"].GetObject()))};
  std::cout << "Camera : " << cameraType << std::endl;
  task->camera = camera_ptr;

  const auto &integratorType = config["integrator"]["type"].GetString();
  std::shared_ptr<Integrator> integrator_ptr{
      static_cast<Integrator *>(ObjectFactory::createInstance(
          integratorType, config["integrator"].GetObject()))};
  std::cout << "Integrator : " << integratorType << std::endl;
  task->integrator = integrator_ptr;

  task->film = std::make_shared<Film>(task->film_size, 32);

  if (config.HasMember("film")) {
    const auto &film_config = config["film"].GetObject();
    std::shared_ptr<Filter> filter = nullptr;
    if (film_config.HasMember("filter")) {
      auto filter_type = film_config["filter"]["type"].GetString();
      filter = std::shared_ptr<Filter>{
          static_cast<Filter *>(ObjectFactory::createInstance(
              filter_type, film_config["filter"].GetObject()))};
    }
    task->film->filter = filter;
  }
}

static std::unordered_map<std::string,
                          std::function<void(std::shared_ptr<RenderTask> task,
                                             const rapidjson::Value &config)>>
    configFunctionTable = {{"output", configureOutPut},
                           {"scene", configureScene},
                           {"renderer", configureRenderer}};

void configureTask(std::shared_ptr<RenderTask> task, const std::string &option,
                   const rapidjson::Value &config) {
  auto configure = configFunctionTable[option];
  configure(task, config);
}

std::shared_ptr<RenderTask> createTask(const std::string &filepath) {
  std::shared_ptr<RenderTask> result = std::make_shared<RenderTask>();

  /*=========== read the file and parse ==============*/
  std::ifstream jsonFile;
  jsonFile.open(filepath, std::ios::in);
  std::streampos begin, end;
  begin = jsonFile.tellg();
  jsonFile.seekg(0, std::ios::end);
  end = jsonFile.tellg();
  jsonFile.seekg(0, std::ios::beg);
  char *json = new char[end - begin];
  jsonFile.read(json, end - begin);
  jsonFile.close();
  rapidjson::Document document;
  document.Parse(json);
  delete[] json;

  for (auto itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr) {
    configureTask(result, itr->name.GetString(), itr->value.GetObject());
  }

  std::cout << "=============================================\n"
            << "=============== Configure done! =============\n"
            << "=============================================\n";

  return result;
}