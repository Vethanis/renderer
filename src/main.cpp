#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "renderobject.h"

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

    HashString sky_xform;
    {
        HashString mesh("suzanne.mesh");
        Material mat = {"basic_diffuse.png", "basic_normal.png"};

        for(float x = -5.0f; x <= 5.0f; x += 2.f){
            for(float y = -5.0f; y <= 5.0f; y += 2.f){

                glm::vec3 pos = glm::vec3(x, y, 0.0f);
                auto& obj = g_Renderables.grow();
                obj.mesh = mesh;
                obj.roughness_offset = (x + 5.0f) / 10.0f;
                obj.metalness_offset = (y + 5.0f) / 10.0f;
                obj.addMaterial(mat);
                Transform* t = obj.transform;
                *t = glm::translate(*t, pos);
            }
        }
        

        auto& obj = g_Renderables.grow();
        obj.mesh = "sphere.mesh";
        obj.addMaterial({"sky_diffuse.png", "sky_normal.png"});
        obj.set_flag(ODF_SKY);
        Transform* t = obj.transform;
        *t = glm::scale(*t, glm::vec3(4.0f));

        sky_xform = obj.transform;
    }
    g_Renderables.finishGrow();

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    u32 flag = DF_DIRECT;
    while(window.open()){
        input.poll(frameBegin(i, t), camera);

        {
            Transform* t = sky_xform;
            (*t)[3] = glm::vec4(camera.getEye().x, camera.getEye().y, camera.getEye().z, 1.0f);
        }

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

        if(input.getKey(GLFW_KEY_UP)){
            g_Renderables.iorr += 0.01f;
        }
        else if(input.getKey(GLFW_KEY_DOWN)){
            g_Renderables.iorr -= 0.01f;
        }

        g_Renderables.mainDraw(camera, flag, WIDTH, HEIGHT);
        window.swap();
    }

    g_Renderables.deinit();

    return 0;
}
