#pragma once

#include "Objects/Checkpoint.h"
#include "Objects/TestObstacle.h"

namespace scene {
void registerEntityTypes(scene::NodeEntityFactory &factory) {
    factory.registerEntity<CheckpointEntity>("CheckpointEntity");
    factory.registerEntity<TestObstacleEntity>("TestObstacleEntity");
}
}  // namespace scene