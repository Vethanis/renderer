#include "texture.h"
#include <cassert>
#include <cstdio>
#include "hashstring.h"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

void ImageStore::load_image(Image& image, unsigned name){
    HashString hstr(name);
    const char* filename = hstr.str();
    assert(filename);
    int channels = 0;
    image.image = stbi_load(filename, &image.width, &image.height, &channels, 4);
    assert(image.image);

    printf("[texture] loaded %s\n", filename);
}

ImageStore g_ImageStore;
TextureStore g_TextureStore;

HashString::operator Image*() const{
    return g_ImageStore[m_hash];
}
HashString::operator Texture*() const{
    return g_TextureStore[m_hash];
}