#pragma once

#include "sdf.h"
#include "store.h"

struct SDFDefinition
{
    SDFList m_sdfs;
    unsigned m_sdfDepth = 0;
};

extern Store<SDFDefinition, 1024> g_SdfStore;