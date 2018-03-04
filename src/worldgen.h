#pragma once

#include "sdf.h"

struct WorldTile
{
    SDFList m_sdfs;
    unsigned m_sdfDepth;

    WorldTile(unsigned name);
    WorldTile()
    {
        m_sdfDepth = 0;
    }
};