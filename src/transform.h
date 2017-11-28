#pragma once

#include "twister.h"
#include "glm/glm.hpp"

typedef glm::mat4 Transform;

extern TwArray<Transform, 1024> g_TransformStore;