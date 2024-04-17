#pragma once

#include <tiny_gltf.h>

#include <functional>
#include <memory>

#include "../Screen.h"

#pragma region ForwardDecl
class LoaderTask;
namespace loader {
class IblEnv;
struct FloatImage;
}  // namespace loader
#pragma endregion

// TODO: This whole class is crap
class GameLoadingScreen : public Screen {
   public:
    struct Data {
        std::shared_ptr<loader::IblEnv> environment;
        std::shared_ptr<loader::IblEnv> environmentSpecular;
        std::shared_ptr<loader::IblEnv> environmentDiffuse;
        std::shared_ptr<loader::FloatImage> iblBrdfLut;

        std::shared_ptr<const tinygltf::Model> gltf;
    };

   private:
    std::unique_ptr<LoaderTask> task_;
    double startTime_;

   public:
    std::function<void(Data& data)> callback;

    GameLoadingScreen();
    virtual ~GameLoadingScreen();

    void draw() override;
};