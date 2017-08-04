#include "texture.h"
#include "filestore.h"
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

void ImageStore::load_image(Image& image, unsigned name){
    const char* filename = g_nameStore.get(name);
    assert(filename);
    int channels = 0;
    image.image = stbi_load(filename, &image.width, &image.height, &channels, 4);
    assert(image.image);
}

ImageStore g_ImageStore;