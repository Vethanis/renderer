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

    HashString building_xform;
    {   
        RenderResource& building = g_Renderables.grow();
        building.mesh = "suzanne.obj";
        building.addMaterial({"grimy_metal_diffuse.png", "grimy_metal_normal.png"});
        building_xform = building.transform;
        auto& sky = g_Renderables.grow();
        sky.mesh = "sphere.obj";
        sky.addMaterial({"sky_diffuse.png", "sky_normal.png"});
        sky.set_flag(DF_SKYMAP);
    }
    g_Renderables.finishGrow();

    {
        Transform* mat = building_xform;
        *mat = glm::scale(*mat, glm::vec3(0.5f));
    }

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    while(window.open()){
        input.poll(frameBegin(i, t), camera);

        {
            Transform* mat = building_xform;
            *mat = glm::rotate(*mat, 0.001f, {0.0f, 1.0f, 0.0f});
        }

        g_Renderables.mainDraw(camera);
        window.swap();
    }

    g_Renderables.deinit();

    return 0;
}
