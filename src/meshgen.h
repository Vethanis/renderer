#pragma once

#include "sdf.h"

struct Geometry;

void CreateMesh(Geometry& output, const SDFList& sdfs, unsigned depth);