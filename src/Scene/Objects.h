#pragma once

#include "Objects/BarrierObstacle.h"
#include "Objects/Checkpoint.h"
#include "Objects/Crate.h"
#include "Objects/MillObstacle.h"

namespace scene {
void registerEntityTypes(scene::NodeEntityFactory &factory) {
    factory.registerEntity<CheckpointEntity>("CheckpointEntity");
    factory.registerEntity<BarrierObstacleEntity>("BarrierObstacleEntity");
    factory.registerEntity<BarrierObstacleEntity>("TestObstacleEntity");  // Deprecated
    factory.registerEntity<MillObstacleEntity>("MillObstacleEntity");
    factory.registerEntity<CrateEntity>("CrateEntity");
}
}  // namespace scene