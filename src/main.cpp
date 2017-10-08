#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "renderobject.h"
#include "gbuffer.h"

#include <random>
#include <ctime>

float randf(void){
    constexpr float inv = 1.0f / float(RAND_MAX);
    return rand() * inv;
}

float randf(float range){
    return randf() * range - (range * 0.5f);
}

float frameBegin(unsigned& i, float& t){
    float dt = (float)glfwGetTime() - t;
    t += dt;
    i++;
    if(t >= 3.0){
        float ms = (t / i) * 1000.0f;
        printf("ms: %.6f, FPS: %.3f\n", ms, i / t);
        i = 0;
        t = 0.0;
        glfwSetTime(0.0);
    }
    return dt;
}

int main(int argc, char* argv[]){
    srand((unsigned)time(0));

    int WIDTH = int(1920.0f * 1.75f);
    int HEIGHT = int(1080.0f * 1.75f);

    if(argc >= 3){
        WIDTH = atoi(argv[1]);
        HEIGHT = atoi(argv[2]);
    }

    Camera camera;
    camera.resize(WIDTH, HEIGHT);
    camera.setEye(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.update();

    Window window(WIDTH, HEIGHT, 4, 5, "Renderer");
    Input input(window.getWindow());

    g_Renderables.init();
    g_gBuffer.init(WIDTH, HEIGHT);

    HashString suzanne;
    {
        const HashString mesh("ball.mesh");
        const TextureChannels channels[] = {
            {"flat_red_albedo.png", "flat_red_material.png"},
            {"basic_albedo.png", "basic_material.png"}
        };

        for(int i = 0; i < 2; ++i){
            for(float x = 0.0f; x < 10.0f; x += 1.0f){
                for(float y = 0.0f; y < 10.0f; y += 1.0f){
                    glm::vec3 pos = glm::vec3(x + 10.0f * float(i), y, 0.0f);

                    g_Renderables.create(mesh, channels[i].albedo, channels[i].material, 
                        glm::translate({}, pos) * glm::scale({}, glm::vec3(0.5f)),
                        x / 10.0f, y / 10.0f);
                }
            }
        }
        
        suzanne = g_Renderables.create("suzanne.mesh", channels[0].albedo, channels[0].material,
            glm::translate({}, glm::vec3(5.0f, 0.0f, 5.0f)));
        g_Renderables.create("suzanne.mesh", channels[1].albedo, channels[1].material,
                glm::translate({}, glm::vec3(10.0f, 0.0f, 5.0f)));
        g_Renderables.create("plane.mesh", "wood_floor_albedo.png", "wood_floor_material.png", 
            glm::scale(glm::translate({}, glm::vec3(5.0f, -3.0f, 0.0f)), glm::vec3(4.0f)));
    }

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    u32 flag = DF_DIRECT;
    while(window.open()){
        input.poll(frameBegin(i, t), camera);

        if(input.getKey(GLFW_KEY_1)){
            flag = DF_INDIRECT;
        }
        else if(input.getKey(GLFW_KEY_2)){
            flag = DF_DIRECT;
        }
        else if(input.getKey(GLFW_KEY_3)){
            flag = DF_REFLECT;
        }
        else if(input.getKey(GLFW_KEY_4)){
            flag = DF_NORMALS;
        }
        else if(input.getKey(GLFW_KEY_5)){
            flag = DF_UV;
        }
        else if(input.getKey(GLFW_KEY_6)){
            flag = DF_VIS_CUBEMAP;
        }
        else if(input.getKey(GLFW_KEY_7)){
            flag = DF_VIS_REFRACT;
        }
        else if(input.getKey(GLFW_KEY_8)){
            flag = DF_VIS_ROUGHNESS;
        }
        else if(input.getKey(GLFW_KEY_9)){
            flag = DF_VIS_METALNESS;
        }

        if(input.getKey(GLFW_KEY_F1)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.roughness_offset;
            x = glm::clamp(x + 0.01f, 0.0f, 1.0f);
        }
        else if(input.getKey(GLFW_KEY_F2)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.roughness_offset;
            x = glm::clamp(x - 0.01f, 0.0f, 1.0f);
        }
        
        if(input.getKey(GLFW_KEY_F3)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.metalness_offset;
            x = glm::clamp(x + 0.01f, 0.0f, 1.0f);
        }
        else if(input.getKey(GLFW_KEY_F4)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.metalness_offset;
            x = glm::clamp(x - 0.01f, 0.0f, 1.0f);
        }

        if(input.getKey(GLFW_KEY_F5)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.bumpiness;
            x = glm::clamp(x + 0.01f, 0.0f, 4.0f);
        }
        else if(input.getKey(GLFW_KEY_F6)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.bumpiness;
            x = glm::clamp(x - 0.01f, 0.0f, 4.0f);
        }

        if(input.getKey(GLFW_KEY_F7)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.index_of_refraction;
            x = glm::clamp(x + 0.01f, 0.001f, 100.0f);
        }
        else if(input.getKey(GLFW_KEY_F8)){
            RenderResource* pRenderable = suzanne;
            float& x = pRenderable->material_params.index_of_refraction;
            x = glm::clamp(x - 0.01f, 0.001f, 100.0f);
        }

        g_gBuffer.draw(camera, flag);
        window.swap();
    }
    
    g_Renderables.deinit();
    g_gBuffer.deinit();

    return 0;
}
