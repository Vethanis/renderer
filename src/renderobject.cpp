#include "renderobject.h"
#include "myglheaders.h"
#include "mesh.h"
#include "glscreen.h"
#include "depthstate.h"
#include "profiler.h"
#include "framebuffer.h"
#include <glm/gtc/matrix_transform.hpp>

Renderables g_Renderables;

void Renderables::init()
{
    ProfilerEvent("Renderables::init");
    
    glEnable(GL_DEPTH_TEST); DebugGL();
    glEnable(GL_CULL_FACE); DebugGL();
    glCullFace(GL_BACK); DebugGL();

    DrawMode::init();

    const char* zFilenames[] = {
        "vert.glsl",
        "zfrag.glsl"
    };
    const char* defFilenames[] = {
        "vert.glsl",
        "write_to_gbuff.glsl"
    };

    zProg.setup(zFilenames, 2);
    defProg.setup(defFilenames, 2);

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
    
    zProg.deinit();
    defProg.deinit();
    m_light.deinit();
}

void Renderables::shadowPass(const Camera& cam)
{
    ProfilerEvent("Renderables::shadowPass");
    
    m_light.drawInto(cam);
}

void Renderables::depthPass(const glm::vec3& eye, const Transform& VP)
{
    ProfilerEvent("Renderables::depthPass");
    
    DepthContext less(GL_LESS);
    DepthMaskContext dmask(GL_TRUE);
    Framebuffer::clear();
    ColorMaskContext nocolor(0);

    zProg.bind();
    defProg.setUniform("eye", eye);
    for(const RenderResource& res : resources)
    {
        const Transform M = glm::scale(glm::translate(Transform(), res.m_field.m_translation), res.m_field.m_scale);
        zProg.setUniform("MVP", VP * M);
        zProg.setUniform("M", M);
        res.draw(zProg);
    }
}

void Renderables::defDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height, u32 target)
{
    ProfilerEvent("Renderables::defDraw");
    
    glViewport(0, 0, width, height); DebugGL();
    Framebuffer::clear();

    defProg.bind();
    defProg.setUniform("eye", eye);
    for(const RenderResource& res : resources)
    {
        const Transform M = glm::scale(glm::translate(Transform(), res.m_field.m_translation), res.m_field.m_scale);
        defProg.setUniform("MVP", VP * M);
        defProg.setUniform("M", M);
        res.draw(defProg);
    }

}