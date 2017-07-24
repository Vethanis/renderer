#include "texture.h"
#include "filestore.h"
#include <cassert>
#include "lodepng.h"

void TextureStore::load_texture(Texture& tex, unsigned name, bool need_init){
    Image* image = m_images[name];

    if(image){
        tex.upload4uc(image->width, image->height, true, image->image);
        return;
    }
    if(m_images.full()){
        image = m_images.reuse_near(name);
        free(image->image);
    }
    else{
        m_images.insert(name, {});
        image = m_images[name];
    }

    const char* filename = g_nameStore.get(name);
    assert(filename);
    unsigned error = lodepng_decode32_file(&image->image, &image->width, &image->height, filename);
    if(error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        assert(false);
    }

    if(need_init){
        tex.init4uc(image->width, image->height, true, image->image);
    }
    else{
        tex.upload4uc(image->width, image->height, true, image->image);
    }

}

TextureStore g_TextureStore;