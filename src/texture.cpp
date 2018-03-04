#include "texture.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "hashstring.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

void Image::load(unsigned name){
    id = name;
    HashString hstr(name);
    const char* filename = hstr.str();
    assert(filename);
    int channels = 0;
    image = stbi_load(filename, &width, &height, &channels, 4);
    assert(image);
}

void Image::free()
{
    stbi_image_free(image);
}

AssetStore<TextureStoreElement, Texture, 32> g_TextureStore;
AssetStore<ImageStoreElement, Image, 128> g_ImageStore;

HashString::operator Image*() const{
    return g_ImageStore[m_hash];
}
HashString::operator Texture*() const{
    return g_TextureStore[m_hash];
}