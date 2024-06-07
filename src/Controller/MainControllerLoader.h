#pragma once

#include "../UI/Screens/Loading.h"
#include "../Util/TaskPool.h"

#pragma region ForwardDecl
namespace loader {
class EnvironmentImage;
struct FloatImage;
struct TerrainData;
}  // namespace loader
namespace tinygltf {
class Model;
class LoadingScreen;
}  // namespace tinygltf

#pragma endregion

class MainControllerLoader {
   public:
    struct Data {
        std::unique_ptr<loader::EnvironmentImage> environment;
        std::unique_ptr<loader::EnvironmentImage> environmentSpecular;
        std::unique_ptr<loader::EnvironmentImage> environmentDiffuse;
        std::unique_ptr<loader::FloatImage> iblBrdfLut;

        std::unique_ptr<const tinygltf::Model> gltf;
        std::unique_ptr<loader::TerrainData> terrain;
    };

   private:
    bool firstTime_ = true;
    std::unique_ptr<TaskPool<Data>> taskPool_;
    std::unique_ptr<LoadingScreen> screen_;
    bool loading_ = false;

    static void queueOperations_(TaskPool<Data>& pool, bool load_gltf);

   public:
    MainControllerLoader();
    ~MainControllerLoader() = default;

    void load();

    void update() {
        // isLoading is called multiple times per frame, but it must always return the same value;
        loading_ = taskPool_ != nullptr && !taskPool_->isFinished();
    }

    bool isLoading() {
        return loading_;
    }

    bool isDone() {
        return !loading_ && taskPool_ != nullptr;
    }

    void draw();

    /**
     * Returns the result and releases the task
     */
    Data result();
};