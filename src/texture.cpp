#include "texture.h"
#include "filestore.h"
#include <cassert>
#include "lodepng.h"

void TextureStore::load_texture(Texture& tex, unsigned name){
    const char* filename = g_nameStore.get(name);
    assert(filename);
    unsigned error;
    unsigned char* image;
    unsigned width, height;

    error = lodepng_decode32_file(&image, &width, &height, filename);
    if(error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        assert(false);
    }

    tex.init4uc(width, height, true);
    tex.upload(image);

    free(image);
}

TextureStore g_TextureStore;