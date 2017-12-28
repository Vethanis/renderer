#include "framebuffer.h"
#include "myglheaders.h"
#include "lodepng.h"
#include "randf.h"

const GLenum attach_ids[Framebuffer::max_attachments] = { 
    GL_COLOR_ATTACHMENT0 + 0,
    GL_COLOR_ATTACHMENT0 + 1, 
    GL_COLOR_ATTACHMENT0 + 2,
    GL_COLOR_ATTACHMENT0 + 3,
    GL_COLOR_ATTACHMENT0 + 4,
    GL_COLOR_ATTACHMENT0 + 5,
    GL_COLOR_ATTACHMENT0 + 6,
    GL_COLOR_ATTACHMENT0 + 7
};

void Framebuffer::init(int width, int height, int num_attachments)
{
    m_width = width;
    m_height = height;

    Assert(num_attachments <= max_attachments && num_attachments > 0);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    m_attachments.resize(num_attachments);
    glGenTextures(num_attachments, m_attachments.begin());

    for(int i = 0; i < m_attachments.count(); ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_attachments[i]); DebugGL();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL); DebugGL();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_attachments[i], 0); DebugGL();
    }
    
    glDrawBuffers(num_attachments, attach_ids); DebugGL();

    glGenRenderbuffers(1, &m_rbo); DebugGL();
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo); DebugGL();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height); DebugGL();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo); DebugGL();

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        puts("Framebuffer not complete!");
        Assert(false);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();
}

void Framebuffer::deinit()
{
    glDeleteRenderbuffers(1, &m_rbo); DebugGL();
    glDeleteTextures(m_attachments.count(), m_attachments.begin()); DebugGL();
    glDeleteFramebuffers(1, &m_fbo); DebugGL();
}

void Framebuffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); DebugGL();
}

void Framebuffer::saveToFile(const char* filename, int attachment)
{
    const int num_components = 4;
    const int num_elems = m_width * m_height * num_components;
    const int row_width = m_width * num_components;
    float* texels = new float[num_elems];
    unsigned char* image = new unsigned char[num_elems];

    glTextureBarrier(); DebugGL();
    glMemoryBarrier(GL_ALL_BARRIER_BITS); DebugGL();
    glBindTexture(GL_TEXTURE_2D, m_attachments[attachment]); DebugGL();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, texels); DebugGL();

    for(int y = 0; y < m_height >> 1; ++y)
    {
        const int y0 = y;
        const int y1 = m_height - y - 1;
        float* rowA = texels + row_width * y0;
        float* rowB = texels + row_width * y1;
        for(int x = 0; x < m_width * num_components; ++x)
        {
            float& A = rowA[x];
            float& B = rowB[x];
            float tmp = A;
            A = B;
            B = tmp;
        }
    }

    u32 s = 72917124;
    for(int i = 0; i < num_elems; ++i)
    {
        float val = texels[i] * 2.0f;
        val = val / (1.0f + val);
        val = powf(val, 1.0f / 2.2f);
        val += randf(s) / 255.0f;
        val *= 255.0f;
        if(val > 255.0f)
            val = 255.0f;
        if(val < 0.0f)
            val = 0.0f;

        image[i] = (unsigned char)val;
    }

    unsigned error = lodepng_encode32_file(filename, image, m_width, m_height);
    if(error)
    {
        puts(lodepng_error_text(error));
    }

    delete[] image;
    delete[] texels;
}

void Framebuffer::bindDefault()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();
}

void Framebuffer::clearDepth()
{
    glClear(GL_DEPTH_BUFFER_BIT); DebugGL();
}

void Framebuffer::clearColor()
{
    glClear(GL_COLOR_BUFFER_BIT) DebugGL();
}

void Framebuffer::clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
}