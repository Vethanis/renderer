#include "transform.h"
#include "hashstring.h"

TransformStore g_TransformStore;

HashString::operator Transform* () const{
    return g_TransformStore[m_hash];
}