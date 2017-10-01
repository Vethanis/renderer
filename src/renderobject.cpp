#include "renderobject.h"

Renderables g_Renderables;

HashString::operator RenderResource*() const{
    return g_Renderables[m_hash];
}