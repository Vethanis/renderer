#pragma once 

#if 0

#include "texture.h"
#include "SSBO.h"
#include "glm/glm.hpp"

// will debug this in a cpu renderer later.

typedef unsigned uint;

namespace MT {
    using namespace glm;

    #define MT_Width 8192
    #define MT_Height 8192
    #define MT_TileSize 128
    #define MSB 0x80000000
    #define MT_MaxDistance 8
    #define MT_MaxMip 5
    #define MT_ImageSize 2048

    const uint MT_NumTiles = ((MT_Width * MT_Height) / (MT_TileSize * MT_TileSize));
    const uint MT_HorizontalTileCount = (MT_Width / MT_TileSize);

    struct store_t {
        uint names[MT_NumTiles];
        uint tiles[MT_NumTiles];
        uint states[MT_NumTiles];
    };

    struct mt_t{
        store_t m_store;
        SSBO name_buf, tile_buf, state_buf;
        inline void init(uint binding){
            name_buf.init(binding);
            tile_buf.init(binding+1);
            state_buf.init(binding+2);
        }
        inline void deinit(){
            name_buf.deinit();
            tile_buf.deinit();
            state_buf.deinit();
        }
        inline void upload(){
            name_buf.upload(m_store.names, sizeof(uint) * MT_NumTiles);
            tile_buf.upload(m_store.tiles, sizeof(uint) * MT_NumTiles);
            state_buf.upload(m_store.states, sizeof(uint) * MT_NumTiles);
        }
        inline void download(){
            name_buf.download(m_store.names, sizeof(uint) * MT_NumTiles);
            tile_buf.download(m_store.tiles, sizeof(uint) * MT_NumTiles);
            state_buf.download(m_store.states, sizeof(uint) * MT_NumTiles);
        }
        inline uint mask(uint key){
            return key & (MT_NumTiles - 1);
        }
        inline uint probe_distance(uint pos, uint key){
            return mask(pos + MT_NumTiles - mask(key));
        }
        inline uint find(uint key){
            uint pos = mask(key);
            uint dist = 0;
            while(true){
                if(names[pos] == 0){
                    return MSB;
                }
                else if(dist > probe_distance(pos, names[pos])){
                    return MSB;
                }
                else if(names[pos] == key){
                    return pos;
                }
                pos = mask(pos + 1);
                ++dist;
            }
            return MSB;
        }
        inline void insert(uint key, uint tile, bool status){
            uint pos = mask(key);
            uint dist = 0;
            while(true){
                if(names[pos] == 0){
                    names[pos] = key;
                    tiles[pos] = tile;
                    states[pos] = status;
                    uvs[pos] = uv;
                    return;
                }

                uint existing_dist = probe_distance(pos, names[pos]);
                if(existing_dist < dist){
                    if(dist >= MT_MaxDistance){
                        names[pos] = key;
                        tiles[pos] = tile;
                        states[pos] = status;
                        return;
                    }

                    uint t_name = names[pos];
                    uint t_tile = tiles[pos];
                    bool t_stat = states[pos];

                    names[pos] = key;
                    tiles[pos] = tile;
                    states[pos] = status;

                    key = t_name;
                    tile = t_tile;
                    status = t_stat;

                    dist = existing_dist;
                }

                pos = mask(pos + 1);
                ++dist;
            }
        }
        inline uint find_or_request(uint key){
            uint pos = find(key);
            if(pos != MSB){
                return pos;
            }
            insert(key, 1);
            return MSB;
        }
        inline uint hash_uint(uint key){
            return 0xfff & (
                    ((key >> 0 ) & 0xfff) ^ 
                    ((key >> 12) & 0xfff) ^ 
                    ((key >> 24) & 0xff )
                );
        }
        inline uvec2 uv_to_tile(vec2 uv, uint mip){
            // [0, 1) -> pixels -> tile
            return uvec2(uv) * ((MT_ImageSize >> mip) / MT_TileSize);
        }
        inline uvec2 tile_to_pixels(uvec2 uv){
            // tile -> mip adjusted pixels
            return uv * MT_TileSize;
        }
        // create a request key
        inline uint to_key(uint mat, vec2 uv, uint mip){
            uvec2 iuv = uv_to_tile(uv, mip);
            uint key = hash_uint(mat) << 20 | 
                ((iuv.x & 0xff) << 12) | 
                ((iuv.y & 0xff) << 4) | 
                (mip & 0xf);
            return key;
        }
        // get which texture was requested (12 bit name)
        inline uint tex_from_key(uint key){
            return 0xfff & (key >> 20);
        }
        // get which mip was requested
        inline uint mip_from_key(uint key){
            return 0xf & key;
        }
        // get which uv was requested
        inline uvec2 uv_from_key(uint key){
            uint x = 0xff & (key >> 12);
            uint y = 0xff & (key >> 4);
            return tile_to_pixels(uvec2(x, y));
        }
        inline uint downgrade_mip(uint key){
            uint mip = mip_from_key(key);
            key &= (~0xf);
            key |= (0xf & (mip + 1));
            return key;
        }
        inline vec4 textureMT(uint mat, vec2 uv){
            // calculate mip using dFd[x,y]Fine(uv) 
            // is not going to be friendly with path tracing since it wont be spatially coherent between threads
            // could try making bounce's samples be more and more averaged to improve convergence and texture occupancy
            // large values => lo frequency
            // small values => hi frequency
            uint pos = 0;
            uint mip = 0;
            uv = fract(abs(uv));
            {
                uint key = to_key(mat, uv, mip);
                pos = find_or_request(key);
                while(pos == MSB && mip < MT_MaxMip){
                    key = downgrade_mip(key);
                    pos = find_or_request(key);
                    ++mip;
                }
                if(pos == MSB || states[pos]){
                    return vec4(0.0);
                }
            }

            uvec2 tile = tile_from_pos(pos);

            // [0, 1) -> mip adjusted pixels
            vec2 virtual_pixel = uv * float(MT_ImageSize >> mip);
            // vector pointing from tile base to pixel position
            uvec2 tvp = mod(virtual_pixel, uvec2(MT_TileSize));

            uvec2 samplerPixel = tile + tvp;
            vec2 dv = fract(virtual_pixel);

            sampler2D sam = getSampler(mat);
            vec4 a = texelFetch(sam, samplerPixel);
            vec4 b = texelFetch(sam, samplerPixel + uvec2(1, 0));
            vec4 c = texelFetch(sam, samplerPixel + uvec2(0, 1));
            vec4 d = texelFetch(sam, samplerPixel + uvec2(1, 1));

            return mix(
                mix(a, b, dv.x),
                mix(c, d, dv.x),
                dv.y
            );
        }
        inline void fulfill_requests(){
            for(uint i = 0; i < MT_NumTiles; i++){
                if(states[i]){
                    uvec2 tile = tile_from_pos(i);
                    uvec2 uv = uv_from_key(names[i]); // mip adjusted pixel
                    uint mip = mip_from_key(names[i]);
                    uint tex = tex_from_key(names[i]);
                    upload_subset(tile, uv, mip, tex);
                    insert()
                    states[i] = false;
                }
            }
        }
    };
};

#endif