
#pragma region ForwardDecl
#include "../../GL/Declarations.h"

struct nk_rect;
struct nk_context;
class Game;
#pragma endregion

class Hud {
   private:
    gl::Texture *crosshairImage_ = nullptr;

    void drawBoostMeter_(Game &game, struct nk_context *nk, struct nk_rect &bounds);
    void drawTimer_(Game &game, struct nk_context *nk, struct nk_rect &bounds);
    void drawCrosshair_(Game &game, struct nk_context *nk, struct nk_rect &bounds);
    void drawVelocity_(Game &game, struct nk_context *nk, struct nk_rect &bounds);

   public:
    Hud();
    ~Hud();
    void draw();
};