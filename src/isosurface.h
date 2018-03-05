#pragma once

#include "linmath.h"
#include "array.h"

struct GRIDCELL
{
   vec3 p[8];
   float val[8];
};

void PolygoniseCell(const GRIDCELL& g, float iso, Vector<vec3>& out);