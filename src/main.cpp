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

    {   
        HashString mesh("suzanne.obj");
        Material mat = {"basic_diffuse.png", "basic_normal.png"};

        for(float x = -10.0f; x <= 10.0f; x += 2.0f){
            for(float z = -10.0f; z <= 10.0f; z += 2.0f){

                glm::vec3 pos = glm::vec3(x, 0.0f, z);
                auto& building = g_Renderables.grow();
                building.mesh = mesh;
                building.addMaterial(mat);
                Transform* t = building.transform;
                *t = glm::translate(*t, pos);
            }
        }
        
    }
    g_Renderables.finishGrow();

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
            flag = DF_DIRECT_REF;
        }
        else if(input.getKey(GLFW_KEY_3)){
            flag = DF_DIRECT;
        }
        else if(input.getKey(GLFW_KEY_4)){
            flag = DF_REFLECT;
        }
        else if(input.getKey(GLFW_KEY_5)){
            flag = DF_NORMALS;
        }
        else if(input.getKey(GLFW_KEY_6)){
            flag = DF_UV;
        }
        else if(input.getKey(GLFW_KEY_7)){
            flag = DF_VIS_CUBEMAP;
        }

        g_Renderables.mainDraw(camera, flag);
        window.swap();
    }

    g_Renderables.deinit();

    return 0;
}
