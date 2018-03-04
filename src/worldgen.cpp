#include "worldgen.h"
#include "randf.h"


WorldTile::WorldTile(unsigned name)
{
    m_sdfs.grow();
    m_sdfDepth = 5;
}