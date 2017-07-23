#include "myglheaders.h"
#include "stdio.h"
#include "camera.h"
#include "debugmacro.h"
#include "window.h"
#include "input.h"
#include "timer.h"
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

    int WIDTH = 1280;
    int HEIGHT = 720;

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

    g_gBuffer.init(WIDTH, HEIGHT);
    g_Renderables.init();

    g_Renderables.add({ glm::mat4(), albedo, spec, mesh });

    Timer timer;

    glEnable(GL_DEPTH_TEST);

    input.poll();
    unsigned i = 0;
    float t = (float)glfwGetTime();
    
    while(window.open()){
        input.poll(frameBegin(i, t), camera);

        g_Renderables.draw(camera.getVP());
        g_gBuffer.draw(camera.getEye());
        window.swap();
    }

    g_Renderables.deinit();
    g_gBuffer.deinit();

    return 0;
}
