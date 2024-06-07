
#include "MainControllerLoader.h"

#include <functional>

#include "../Loader/Environment.h"
#include "../Loader/Gltf.h"
#include "../Loader/Terrain.h"
#include "../Util/Log.h"

MainControllerLoader::MainControllerLoader() {
    screen_ = std::make_unique<LoadingScreen>();
}

void MainControllerLoader::queueOperations_(TaskPool<Data>& pool, bool load_gltf) {
    if (load_gltf) {
        pool.add([](Data& out) {
            out.gltf = std::make_unique<tinygltf::Model>(
                loader::gltf("assets/models/test_course.glb"));
        });
    }

    pool.add([](Data& out) {
        out.environment = std::unique_ptr<loader::EnvironmentImage>(
            loader::environment("assets/textures/skybox/kloofendal.iblenv"));
    });

    pool.add([](Data& out) {
        out.environmentDiffuse = std::unique_ptr<loader::EnvironmentImage>(
            loader::environment("assets/textures/skybox/kloofendal_diffuse.iblenv"));
    });

    pool.add([](Data& out) {
        out.environmentSpecular = std::unique_ptr<loader::EnvironmentImage>(
            loader::environment("assets/textures/skybox/kloofendal_specular.iblenv"));
    });

    pool.add([](Data& out) {
        out.iblBrdfLut = std::unique_ptr<loader::FloatImage>(
            loader::floatImage("assets/textures/ibl_brdf_lut.f32"));
    });

    pool.add([](Data& out) {
        out.terrain = std::make_unique<loader::TerrainData>(
            loader::TerrainData::Files{
                .albedo = "assets/textures/terrain_albedo.dds",
                .height = "assets/textures/terrain_height.png",
                .occlusion = "assets/textures/terrain_ao.png",
                .normal = "assets/textures/terrain_normal.png",
            });
    });
}

void MainControllerLoader::load() {
    taskPool_ = std::make_unique<TaskPool<Data>>();
    queueOperations_(*taskPool_, firstTime_);
    loading_ = true;
    if (firstTime_) {
        screen_->open(taskPool_.get());
        taskPool_->runAsync();
    } else {
        taskPool_->runSync();
    }
    firstTime_ = false;
}

void MainControllerLoader::draw() {
    if (taskPool_ == nullptr) {
        return;
    }
    screen_->draw();
}

MainControllerLoader::Data MainControllerLoader::result() {
    Data data = std::move(taskPool_->result());
    taskPool_.reset();
    screen_->close();
    return data;
}