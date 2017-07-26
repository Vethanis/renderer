#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "renderobject.h"
#include "filestore.h"

#include <random>
#include <ctime>

using namespace glm;



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
    store_test();
    
    srand((unsigned)time(0));
    unsigned albedo = g_nameStore.add("brick_diffuse.png");
    unsigned normal = g_nameStore.add("brick_normal.png");
    unsigned mesh = g_nameStore.add("building.obj");

    int WIDTH = int(1920.0f * 1.5f);
    int HEIGHT = int(1080.0f * 1.5f);

    if(argc >= 3){
        WIDTH = atoi(argv[1]);
        HEIGHT = atoi(argv[2]);
    }

    Camera camera;
    camera.resize(WIDTH, HEIGHT);
    camera.setEye(vec3(0.0f, 0.0f, 3.0f));
    camera.update();

    Window window(WIDTH, HEIGHT, 3, 3, "Renderer");
    Input input(window.getWindow());

    g_Renderables.init();
    unsigned building = g_Renderables.add({});
    g_Renderables[building].resource.mesh = mesh;
    g_Renderables[building].resource.add({albedo, normal});
    g_gBuffer.init(WIDTH, HEIGHT);

    LightSet lights;

    const auto randomizeLights = [&](){
        for(int i = 0; i < 32; i++){
            lights[i].position.x = randf(25.0f);
            lights[i].position.y = randf() * 25.0f;
            lights[i].position.z = randf(25.0f);
            lights[i].color.x = randf();
            lights[i].color.y = randf();
            lights[i].color.z = randf();
        }
        g_gBuffer.updateLights(lights);
    };

    randomizeLights();

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    int wait_counter = 0;
    while(window.open()){
        input.poll(frameBegin(i, t), camera);

        if(glfwGetKey(window.getWindow(), GLFW_KEY_E) && wait_counter > 10){
            randomizeLights();
            wait_counter = 0;
        }

        g_gBuffer.draw(camera);
        window.swap();
        wait_counter++;
    }

    g_Renderables.deinit();
    g_gBuffer.deinit();

    return 0;
}
