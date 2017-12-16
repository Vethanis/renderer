#include "renderobject.h"
#include "myglheaders.h"
#include "mesh.h"
#include "glscreen.h"
#include "depthstate.h"
#include <random>
#include "profiler.h"

Renderables g_Renderables;

void Renderables::init()
{
    ProfilerEvent("Renderables::init");
    
    glEnable(GL_DEPTH_TEST); DebugGL();
    glEnable(GL_PROGRAM_POINT_SIZE); DebugGL();

    DrawMode::init();

    const char* fwdFilenames[] = {
        "vert.glsl",
        "fwdFrag.glsl"
    };
    const char* zFilenames[] = {
        "zvert.glsl",
        "zfrag.glsl"
    };
    const char* defFilenames[] = {
        "vert.glsl",
        "write_to_gbuff.glsl"
    };

    fwdProg.setup(fwdFilenames, 2);
    zProg.setup(zFilenames, 2);
    defProg.setup(defFilenames, 2);

    skyProg.init(); 
    skyProg.addShader(GLScreen::vertexShader());
    int shader = skyProg.addShader("skyfrag.glsl", GL_FRAGMENT_SHADER);
    skyProg.link();
    skyProg.freeShader(shader);

    m_light.init(1024);
    m_light.m_direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    m_light.m_color = glm::vec3(1.0f, 0.75f, 0.5f);
    m_light.m_position = m_light.m_direction * 50.0f;
    m_light.m_intensity = 10.0f;
    m_light.m_near = 10.0f;
    m_light.m_far = 100.0f;
}

void Renderables::deinit()
{
    ProfilerEvent("Renderables::deinit");
    
    fwdProg.deinit();
    zProg.deinit();
    defProg.deinit();
    skyProg.deinit();
    m_light.deinit();
}

void Renderables::bindSun(DirectionalLight& light, GLProgram& prog, int channel)
{
    ProfilerEvent("Renderables::bindSun");
    
    prog.setUniform("sunDirection", light.m_direction);
    prog.setUniform("sunColor", light.m_color);
    prog.setUniformFloat("sunIntensity", light.m_intensity);
    prog.setUniform("sunMatrix", light.m_matrix);
    prog.bindTexture(channel, light.m_tex, "sunDepth");
}

void Renderables::drawSky(const glm::vec3& eye, const Transform& IVP)
{
    ProfilerEvent("Renderables::drawSky");
    
    DrawModeContext ctx(GL_LEQUAL, GL_TRUE, 1);
    skyProg.bind();
    
    skyProg.setUniform("IVP", IVP);
    skyProg.setUniform("eye", eye);
    skyProg.setUniform("sunDirection", m_light.m_direction);
    skyProg.setUniformFloat("sunIntensity", m_light.m_intensity);

    GLScreen::draw();
}

void Renderables::shadowPass()
{
    ProfilerEvent("Renderables::shadowPass");
    
    m_light.drawInto();
}

void Renderables::prePass(const Transform& VP)
{
    ProfilerEvent("Renderables::prePass");
    
    DepthContext less(GL_LESS);
    DepthMaskContext dmask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
    ColorMaskContext nocolor(0);

    zProg.bind();
    for(const RenderResource& res : resources)
    {
        zProg.setUniform("MVP", VP * res.m_transform);
        res.draw();
    }
}

void Renderables::fwdDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height)
{
    ProfilerEvent("Renderables::fwdDraw");
    
    glViewport(0, 0, width, height); DebugGL();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();

    fwdProg.bind();

    bindSun(m_light, fwdProg, 10);

    fwdProg.setUniform("eye", eye);
    fwdProg.setUniformInt("seed", rand());
    
    for(const RenderResource& res : resources)
    {
        const Transform& M = res.m_transform;
        const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(M)));

        fwdProg.setUniform("MVP", VP * M);
        fwdProg.setUniform("M", M);
        fwdProg.setUniform("IM", IM);

        res.draw();
    }

    drawSky(eye, glm::inverse(VP));
}

void Renderables::defDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height)
{
    ProfilerEvent("Renderables::defDraw");
    
    glViewport(0, 0, width, height); DebugGL();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();

    defProg.bind();

    for(const RenderResource& res : resources)
    {
        const Transform& M = res.m_transform;
        const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(M)));

        defProg.setUniform("MVP", VP * M);
        defProg.setUniform("M", M);
        defProg.setUniform("IM", IM);

        res.draw();
    }
}

u16 Renderables::request()
{
    const u16 handle =  resources.insert({});
    resources[handle].init();
    return handle;
}

void Renderables::release(u16 handle)
{
    resources[handle].deinit();
    resources.remove(handle);
}