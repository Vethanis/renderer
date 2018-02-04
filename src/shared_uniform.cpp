#include "shared_uniform.h"
#include "SSBO.h"
#include "randf.h"

SharedUniforms g_sharedUniforms;

SSBO ssbo;

void InitializeSharedUniforms()
{
    ssbo.init(SHARED_UNIFORMS_BINDING);
    ssbo.upload(&g_sharedUniforms, sizeof(SharedUniforms));
}

void NotifySharedUniformsUpdated()
{
    g_sharedUniforms.seed_flags.x = s32(randu());
    ssbo.upload(&g_sharedUniforms, sizeof(SharedUniforms));
}

void ShutdownSharedUniforms()
{
    ssbo.deinit();
}
