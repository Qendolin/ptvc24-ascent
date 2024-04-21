#include "../Util/Timer.h"
#include "Entity.h"

#pragma region ForwardDecl
class Camera;
#pragma endregion

// allows free camera movement for debugging
class FreeCamEntity : public scene::Entity {
   private:
    // Flying speed in m/s
    inline static const float BASE_SPEED = 10.0f;
    inline static const float MIN_SPEED = 0.1f;
    inline static const float MAX_SPEED = 100.0f;
    // Change speed by scrolling
    float speed_ = BASE_SPEED;

   public:
    Camera& camera;

    FreeCamEntity(Camera& camera) : Entity(scene::SceneRef{}), camera(camera) {
    }

    virtual ~FreeCamEntity() = default;

    void init() override;

    void update(float time_delta) override;
};