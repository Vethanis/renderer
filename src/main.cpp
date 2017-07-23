#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "renderobject.h"
#include "filestore.h"

using namespace glm;

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

    unsigned albedo = g_nameStore.add("albedo.png");
    unsigned spec = g_nameStore.add("specular.png");
    unsigned mesh = g_nameStore.add("suzanne.ply");

    int WIDTH = 1920;
    int HEIGHT = 1080;

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

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    g_Renderables.init();
    g_Renderables.add({ glm::mat4(), albedo, spec, mesh });
    g_gBuffer.init(WIDTH, HEIGHT);

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    
    LightSet lights;
    lights[0].color = vec4(1.0f);
    while(window.open()){
        input.poll(frameBegin(i, t), camera);
        lights[0].position = vec4(camera.getEye(), 0.0f);
        g_gBuffer.updateLights(lights);

        g_gBuffer.draw(camera);
        window.swap();
    }

    g_Renderables.deinit();
    g_gBuffer.deinit();

    return 0;
}
