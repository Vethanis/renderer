#include "renderobject.h"
#include "mesh.h"
#include "texture.h"
#include "glscreen.h"
#include "depthstate.h"
#include <random>
#include "profiler.h"

Renderables g_Renderables;

void TextureChannels::bind(GLProgram& prog, int channel)
{
    ProfilerEvent("TextureChannels::bind");

    Texture* t = albedo;
    if(t)
    {
        prog.bindTexture(TX_ALBEDO_CHANNEL, t->handle, "albedoSampler");
    }
    t = material;
    if(t)
    {
        prog.bindTexture(TX_MATERIAL_CHANNEL, t->handle, "materialSampler");
    }
}

void RenderResource::init()
{
    transform = g_TransformStore.insert({});
    m_uv_scale = glm::vec2(1.0f);
}

void RenderResource::deinit()
{
    g_TransformStore.remove(transform);
}

void RenderResource::bind(GLProgram& prog, UBO& material_ubo)
{
    ProfilerEvent("RenderResource::bind");
    
    texture_channels.bind(prog, 0);
    material_ubo.upload(&material_params, sizeof(MaterialParams));
}

void RenderResource::draw()
{
    ProfilerEvent("RenderResource::draw");
    
    Mesh* m = mesh;
    if(m){
        m->draw();
    }
}

void RenderResource::setTransform(const Transform& xform)
{
    Transform& rXform = g_TransformStore[transform];
    rXform = xform;
}

void RenderResource::setVelocity(const glm::vec3& dv)
{
    m_prevVelocity = m_velocity;
    m_velocity = dv;
}

void Renderables::init()
{
    ProfilerEvent("Renderables::init");
    
    glEnable(GL_DEPTH_TEST); DebugGL();
    glEnable(GL_CULL_FACE); DebugGL();
    glCullFace(GL_BACK); DebugGL();

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

    materialparam_ubo.init(nullptr, sizeof(MaterialParams), "materialparams_ubo", &fwdProg.m_id, 1);
}

void Renderables::deinit()
{
    ProfilerEvent("Renderables::deinit");
    
    fwdProg.deinit();
    zProg.deinit();
    defProg.deinit();
    skyProg.deinit();
    materialparam_ubo.deinit();
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
    for(auto& res : resources){
        Transform& M = g_TransformStore[res.transform];
        zProg.setUniform("MVP", VP * M);
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
    fwdProg.setUniformInt("draw_flags", dflag);
    
    for(auto& res : resources){
        Transform& M = g_TransformStore[res.transform];
        const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(M)));

        fwdProg.setUniform("MVP", VP * M);
        fwdProg.setUniform("M", M);
        fwdProg.setUniform("IM", IM);

        res.bind(fwdProg, materialparam_ubo);
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
    defProg.setUniform("eye", eye);
    defProg.setUniformInt("draw_flags", dflag);

    for(auto& res : resources){
        Transform& M = g_TransformStore[res.transform];
        const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(M)));

        defProg.setUniform("MVP", VP * M);
        defProg.setUniform("M", M);
        defProg.setUniform("IM", IM);
        defProg.setUniform("velocity", res.m_prevVelocity);
        defProg.setUniform("uv_scale", res.m_uv_scale);
        defProg.setUniform("uv_offset", res.m_uv_offset);

        res.bind(defProg, materialparam_ubo);
        res.draw();
    }
}

u16 Renderables::grow()
{
    u16 handle = resources.insert({});
    resources[handle].init();
    return handle;
}

void Renderables::release(u16 handle)
{
    resources[handle].deinit();
    resources.remove(handle);
}

u16 Renderables::create(HashString mesh, HashString albedo, 
    HashString material, const Transform& xform, 
    float roughness, float metalness, 
    unsigned flags)
{
    ProfilerEvent("Renderables::create");

    u16 handle = resources.insert({});
    RenderResource& res = resources[handle];
    res.init();
    res.mesh = mesh;
    res.texture_channels.albedo = albedo;
    res.texture_channels.material = material;
    res.material_params.roughness_offset = roughness;
    res.material_params.metalness_offset = metalness;
    res.setTransform(xform);

    return handle;
}