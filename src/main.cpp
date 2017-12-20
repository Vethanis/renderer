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
#include "meshgen.h"

#include <random>
#include <ctime>

void setupScene()
{
    using namespace glm;

    MeshTask task;
    task.center = vec3(5.0f, 5.0f, 0.0f);
    task.radius = 10.0f;
    task.max_depth = 6;

    for(float x = 0.0f; x < 10.0f; x += 2.25f)
    {
        for(float y = 0.0f; y < 10.0f; y += 1.5f)
        {
            SDF& sdf = task.sdfs.grow();
            if(x > y)
                sdf.type = SDF_BOX;
                
            sdf.translation = vec3(x, y, 0.0f);
            sdf.material.setRoughness(glm::clamp(x / 10.0f, 0.15f, 1.0f));
            sdf.material.setMetalness(y / 10.0f);
            sdf.material.setColor(vec3(1.0f, 0.0f, 0.0f));
        }
    }
    GenerateMesh(task);

    u16 handle = g_Renderables.request();
    RenderResource& res = g_Renderables[handle];
    res.mesh.upload(task.geom);
}

void FpsStats()
{
    if((frameCounter() & 127) == 0)
    {
        const double ms = frameSeconds() * 1000.0;
        printf("ms: %.6f, FPS: %.3f\n", ms, 1000.0 / ms);
    }
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
    input.poll();
    u32 flag = DF_INDIRECT;
    ProfilerInit();

    setupScene();

    while(window.open())
    {
        ProfilerEvent("MainLoop");
        input.poll(camera);

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
                    g_Renderables.m_light.m_position = camera.getEye() + camera.getAxis() * 50.0f;
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
                case GLFW_KEY_KP_0: flag = DF_VIS_SHADOW_BUFFER; break;
                case GLFW_KEY_V: flag = DF_VIS_SUN_SHADOW_DEPTH; break;
                case GLFW_KEY_L: flag = DF_VIS_LDN; break;
                case GLFW_KEY_O: flag = DF_VIS_AO; break;
                case GLFW_KEY_F12:
                {
                    g_gBuffer.screenshot();
                }
                break;
            }
        }

        g_gBuffer.draw(camera, flag);

        window.swap();
        FpsStats();
    }
    
    g_Renderables.deinit();
    g_gBuffer.deinit();
    ProfilerDeinit();

    return 0;
}
