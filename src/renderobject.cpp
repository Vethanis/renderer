
#include "renderobject.h"
#include "myglheaders.h"
#include "mesh.h"
#include "glscreen.h"
#include "depthstate.h"
#include "profiler.h"
#include "framebuffer.h"
#include <glm/gtc/matrix_transform.hpp>
#include "shared_uniform.h"
#include "randf.h"
#include "camera.h"

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
    const char* fwdFilenames[] = 
    {
        "fwdVert.glsl",
        "fwdFrag.glsl"
    };

    fwdProg.setup(fwdFilenames, 2);
    zProg.setup(zFilenames, 2);

    m_light.init(1024);
    m_light.m_direction = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
    m_light.m_color = glm::vec3(1.0f, 0.75f, 0.5f);
    m_light.m_position = m_light.m_direction * 50.0f;
    m_light.m_intensity = 10.0f;
    m_light.m_near = 10.0f;
    m_light.m_far = 100.0f;
    
    InitializeSharedUniforms();
    InitRasterFields();
    ProfilerInit();
}

void Renderables::deinit()
{
    ProfilerEvent("Renderables::deinit");
    
    zProg.deinit();
    fwdProg.deinit();
    m_light.deinit();

    ShutdownSharedUniforms();
    ProfilerDeinit();
}

void Renderables::shadowPass(const Camera& cam)
{
    ProfilerEvent("Renderables::shadowPass");

    g_sharedUniforms.sunMatrix = m_light.m_matrix;
    g_sharedUniforms.sunDirection = vec4(m_light.m_direction.x, m_light.m_direction.y, m_light.m_direction.z, g_sharedUniforms.sunDirection.w);
    g_sharedUniforms.sunColor = vec4(m_light.m_color.x, m_light.m_color.y, m_light.m_color.z, m_light.m_intensity);
    
    depthPass(cam.getEye(), cam.getVP());
}

void Renderables::depthPass(const glm::vec3& eye, const mat4& VP)
{
    ProfilerEvent("Renderables::depthPass");
    
    DepthContext less(GL_LESS);
    DepthMaskContext dmask(GL_TRUE);
    Framebuffer::clear();
    ColorMaskContext nocolor(0);

    g_sharedUniforms.MVP = VP;
    g_sharedUniforms.eye = vec4(eye.x, eye.y, eye.z, g_sharedUniforms.eye.w);

    zProg.bind();
    for(const RenderResource& res : resources)
    {
        res.draw(zProg);
    }
}

void Renderables::fwdPass(const glm::vec3& eye, const mat4& VP, u32 dflag)
{
    ProfilerEvent("Renderables::fwdPass");
    
    DrawModeContext defaultCtx;
    Framebuffer::clear();

    g_sharedUniforms.MVP = VP;
    g_sharedUniforms.IVP = glm::inverse(VP);
    g_sharedUniforms.eye = vec4(eye.x, eye.y, eye.z, g_sharedUniforms.eye.w);
    g_sharedUniforms.seed_flags.y = s32(dflag);

    g_sharedUniforms.sunMatrix = m_light.m_matrix;
    g_sharedUniforms.sunDirection = vec4(m_light.m_direction.x, m_light.m_direction.y, m_light.m_direction.z, g_sharedUniforms.sunDirection.w);
    g_sharedUniforms.sunColor = vec4(m_light.m_color.x, m_light.m_color.y, m_light.m_color.z, m_light.m_intensity);

    fwdProg.bind();
    for(const RenderResource& res : resources)
    {
        res.draw(fwdProg);
    }
}