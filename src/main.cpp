#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "renderobject.h"
#include "gbuffer.h"
#include "timer.h"
#include "framecounter.h"
#include "profiler.h"

#include <random>
#include <ctime>

float FpsStats()
{
    static float last_time = (float)glfwGetTime();
    static float average_dt = 0.0f;
    const float cur_time = (float)glfwGetTime();
    const float dt = cur_time - last_time;
    last_time = cur_time;
    average_dt += dt;

    if((frameCounter() & 63) == 0)
    {
        average_dt /= 64.0f;
        float ms = average_dt * 1000.0f;
        printf("ms: %.6f, FPS: %.3f\n", ms, 1.0f / average_dt);
        average_dt = 0.0f;
    }

    return dt;
}

int main(int argc, char* argv[])
{
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
                glm::translate({}, glm::vec3(10.0f, -2.1f, 5.0f)));
        HashString plane = g_Renderables.create("cube.mesh", "wood_floor_albedo.png", "wood_floor_material.png", 
            glm::scale(glm::translate({}, glm::vec3(5.0f, -3.0f, 0.0f)), glm::vec3(16.0f, 0.1f, 16.0f)));
        RenderResource* pRes = plane;
        if(pRes)
        {
            pRes->m_uv_scale *= 16.0f;
        }
    }

    input.poll();
    u32 flag = DF_INDIRECT;

    Timer timer;
    while(window.open())
    {
        ProfilerEvent("Main Loop");
        
        input.poll(FpsStats(), camera);

        RenderResource* pRes = suzanne;
        if(pRes)
        {
            Transform* xform = pRes->transform;
            if(xform)
            {
                float phase = frameCounter() / 10.0f;
                phase = glm::mod(phase, 3.141592f * 2.0f);
                float x = glm::sin(phase) * 0.05f;
                float y = glm::cos(phase) * 0.05f;
                glm::vec3 dv = glm::vec3(x, y, 0.0f);
                *xform = glm::translate(*xform, dv);
                pRes->setVelocity(dv);
            }
        }

        for(int key : input)
        {
            switch(key)
            {
                case GLFW_KEY_KP_ADD:
                {

                }
                break;
                case GLFW_KEY_KP_SUBTRACT:
                {
                    
                }
                break;
                case GLFW_KEY_E:
                {
                    g_Renderables.m_light.m_direction = camera.getAxis();
                }
                break;
                case GLFW_KEY_1:
                {
                    flag = DF_INDIRECT;
                }
                break;
                case GLFW_KEY_2:
                {
                    flag = DF_DIRECT;
                }
                break;
                case GLFW_KEY_3:
                {
                    flag = DF_REFLECT;
                }
                break;
                case GLFW_KEY_4:
                {
                    flag = DF_NORMALS;
                }
                break;
                case GLFW_KEY_5:
                {
                    flag = DF_UV;
                }
                break;
                case GLFW_KEY_6:
                {
                    flag = DF_VIS_CUBEMAP;
                }
                break;
                case GLFW_KEY_7:
                {
                    flag = DF_VIS_REFRACT;
                }
                break;
                case GLFW_KEY_8:
                {
                    flag = DF_VIS_ROUGHNESS;
                }
                break;
                case GLFW_KEY_9:
                {
                    flag = DF_VIS_METALNESS;
                }
                break;
                case GLFW_KEY_0: flag = DF_VIS_VELOCITY; break;
                case GLFW_KEY_T:
                {
                    flag = DF_VIS_TANGENTS;
                }
                break;
                case GLFW_KEY_B:
                {
                    flag = DF_VIS_BITANGENTS;
                }
                break;
                case GLFW_KEY_V: flag = DF_VIS_SUN_SHADOW_DEPTH; break;
                case GLFW_KEY_F1:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.roughness_offset;
                    x = glm::clamp(x + 0.01f, 0.0f, 1.0f);
                }
                break;
                case GLFW_KEY_F2:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.roughness_offset;
                    x = glm::clamp(x - 0.01f, 0.0f, 1.0f);
                }
                break;
                case GLFW_KEY_F3:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.metalness_offset;
                    x = glm::clamp(x + 0.01f, 0.0f, 1.0f);
                }
                break;
                case GLFW_KEY_F4:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.metalness_offset;
                    x = glm::clamp(x - 0.01f, 0.0f, 1.0f);
                }
                break;
                case GLFW_KEY_F5:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.bumpiness;
                    x = glm::clamp(x + 0.01f, 0.0f, 4.0f);
                }
                break;
                case GLFW_KEY_F6:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.bumpiness;
                    x = glm::clamp(x - 0.01f, 0.0f, 4.0f);
                }
                break;
                case GLFW_KEY_F7:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.index_of_refraction;
                    x = glm::clamp(x + 0.01f, 0.001f, 100.0f);
                }
                break;
                case GLFW_KEY_F8:
                {
                    RenderResource* pRenderable = suzanne;
                    float& x = pRenderable->material_params.index_of_refraction;
                    x = glm::clamp(x - 0.01f, 0.001f, 100.0f);
                }
                break;
                case GLFW_KEY_F12:
                {
                    g_gBuffer.screenshot();
                }
                break;
            }
        }

        g_gBuffer.draw(camera, flag);

        window.swap();
    }
    
    g_Renderables.deinit();
    g_gBuffer.deinit();

    FinishProfiling("profile_results.csv");

    return 0;
}
