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
#include "assetstore.h"
#include "worldgen.h"
#include "randf.h"
#include <ctime>
#include "texture.h"

unsigned scene_mesh_id = 5;
u16 curRenderable;
bool hasRenderable = false;
bool hasRequestedAllTextures = false;
void SceneSetup(vec3 pt, float radius, int matId, bool makeNew)
{
    HashString albedo("basic_albedo.png");
    HashString material("basic_material.png");
    HashString albedo2("wood_floor_albedo.png");
    HashString material2("wood_floor_material.png");
    if(matId & 1)
    {
        albedo = albedo2;
        material = material2;
    }
    if(!hasRequestedAllTextures)
    {
        g_TextureStore.request(albedo);
        g_TextureStore.request(albedo2);
        g_TextureStore.request(material);
        g_TextureStore.request(material2);
    }

    HashString mesh = scene_mesh_id;
    if(makeNew)
    {
        g_SdfStore.remove(scene_mesh_id);
        ++scene_mesh_id;
        mesh = scene_mesh_id;
        g_SdfStore.insert(mesh, {});
    }
    else
    {
        if(hasRenderable)
        {
            g_Renderables.release(curRenderable);
            hasRenderable = false;
        }
    }

    SDFDefinition* pDef = g_SdfStore[mesh];
    if(!pDef)
    {
        g_SdfStore.insert(mesh, {});
        pDef = g_SdfStore[mesh];
    }
    pDef->m_sdfDepth = 7;

    SDFList& list = pDef->m_sdfs;
    SDF& sdf = list.grow();
    sdf.translation = pt;
    sdf.scale = vec3(radius);
    sdf.blend_type = SDF_S_UNION;
    sdf.smoothness = radius * 0.5f;

    curRenderable = g_Renderables.create(mesh, albedo, material);
    hasRenderable = true;
}

void FpsStats()
{
    static double average_dt = 0.0;
    average_dt += frameSeconds();

    if((frameCounter() & 63) == 0)
    {
        average_dt /= 64.0;
        const double ms = average_dt * 1000.0;
        printf("ms: %.6f, FPS: %.3f\n", ms, 1.0 / average_dt);
        average_dt = 0.0;
    }
}

int main(int argc, char* argv[])
{
    g_randSeed = (unsigned)time(NULL);

    int WIDTH = int(1920.0f * 1.5f);
    int HEIGHT = int(1080.0f * 1.5f);

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

    int curMat = 0;
    float radius = 1.0f;
    SceneSetup(camera.getAxis() * 5.0f + camera.getEye(), radius, curMat, false);

    while(window.open())
    {
        ProfilerEvent("Main Loop");
        input.poll(camera);

        for(int key : input)
        {
            switch(key)
            {
                case GLFW_KEY_UP:
                {
                    radius *= 1.01f;
                }
                break;
                case GLFW_KEY_DOWN:
                {
                    radius *= 0.99f;
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
                case GLFW_KEY_O: flag = DF_VIS_AMBIENT_OCCLUSION; break;
                case GLFW_KEY_F12:
                {
                    g_gBuffer.screenshot();
                }
                break;
            }
        }

        for(const int* pKey = input.downBegin(); pKey != input.downEnd(); ++pKey)
        {
            switch(*pKey)
            {
                case GLFW_KEY_F1:
                    SceneSetup(camera.getAxis() * 5.0f + camera.getEye(), radius, curMat, false);
                break;
                case GLFW_KEY_F2:
                    SceneSetup(camera.getAxis() * 5.0f + camera.getEye(), radius, curMat, true);
                break;
                case GLFW_KEY_F3:
                    curMat++;
                    break;
                case GLFW_KEY_F4:
                    curMat--;
                    break;
            }
        }

        UpdateAssetStores();
        g_gBuffer.draw(camera, flag);

        window.swap();
        FpsStats();
        ProfilerEndFrame();
    }
    
    g_Renderables.deinit();
    g_gBuffer.deinit();

    FinishProfiling("profile_results.csv");
    ProfilerDeinit();

    return 0;
}
