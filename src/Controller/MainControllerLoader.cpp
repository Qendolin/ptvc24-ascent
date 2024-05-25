
#include "MainControllerLoader.h"

#include "../Loader/Environment.h"
#include "../Loader/Gltf.h"
#include "../Loader/Terrain.h"
#include "../Util/Log.h"

MainControllerLoader::MainControllerLoader() {
    screen_ = std::make_unique<LoadingScreen>();
}

void MainControllerLoader::load_(Data& out, bool load_gltf) {
    if (load_gltf) {
        out.gltf = std::make_unique<tinygltf::Model>(
            loader::gltf("assets/models/test_course.glb"));
    }

    out.environment = std::unique_ptr<loader::EnvironmentImage>(
        loader::environment("assets/textures/skybox/kloofendal.iblenv"));
    out.environmentDiffuse = std::unique_ptr<loader::EnvironmentImage>(
        loader::environment("assets/textures/skybox/kloofendal_diffuse.iblenv"));
    out.environmentSpecular = std::unique_ptr<loader::EnvironmentImage>(
        loader::environment("assets/textures/skybox/kloofendal_specular.iblenv"));
    out.iblBrdfLut = std::unique_ptr<loader::FloatImage>(
        loader::floatImage("assets/textures/ibl_brdf_lut.f32"));

    out.terrain = std::make_unique<loader::TerrainData>(
        loader::TerrainData::Files{
            .albedo = "assets/textures/terrain_albedo.jpg",
            .height = "assets/textures/terrain_height.png",
            .occlusion = "assets/textures/terrain_ao.png",
            .normal = "assets/textures/terrain_normal.png",
        });
}

void MainControllerLoader::load() {
    task_ = std::make_unique<Task<Data>>(load_, firstTime_);
    loading_ = true;
    if (firstTime_) {
        screen_->open(task_.get());
        task_->runAsync();
    } else {
        task_->runSync();
    }
    firstTime_ = false;
}

void MainControllerLoader::draw() {
    if (task_ == nullptr) {
        return;
    }
    screen_->draw();
}

MainControllerLoader::Data MainControllerLoader::result() {
    Data data = std::move(task_->result());
    task_.reset();
    screen_->close();
    return data;
}