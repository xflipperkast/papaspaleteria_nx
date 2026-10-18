#pragma once
#include "port_controls.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct PaleteriaCursorGl {
    EGLContext context;
    EGLDisplay display;
    EGLSurface surface;
    GLuint program,buffer,quad,texture,resolve_fbo,vao;
    GLint transform,fill,background,sampler;
    int width,height;
    uint32_t last_cursor;
    bool cached,reverse;
    GLsizei vertices;
    bool es3;
} PaleteriaCursorGl;
static PaleteriaCursorGl paleteria_cursor_gl[8];
static pthread_mutex_t paleteria_cursor_gl_lock=PTHREAD_MUTEX_INITIALIZER;

static GLuint paleteria_cursor_shader(GLenum type,const char *source) {
    GLuint shader=glCreateShader(type);GLint ok=0;
    if(!shader)return 0;
    glShaderSource(shader,1,&source,NULL);glCompileShader(shader);
    glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok){glDeleteShader(shader);APK64_LOGCW("CURSOR","Cursor shader compilation failed");return 0;}
    return shader;
}
static void paleteria_cursor_gl_forget(EGLContext context) {
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    for(size_t i=0;i<8;++i)if(paleteria_cursor_gl[i].context==context) {
        PaleteriaCursorGl *g=&paleteria_cursor_gl[i];
        if(eglGetCurrentContext()==context) {
            glDeleteProgram(g->program);glDeleteBuffers(1,&g->buffer);glDeleteBuffers(1,&g->quad);
            glDeleteTextures(1,&g->texture);
            glDeleteFramebuffers(1,&g->resolve_fbo);
            if(g->vao)glDeleteVertexArrays(1,&g->vao);
        }
        /* shortcut: non-current shared objects wait for share-group teardown. */
        memset(g,0,sizeof(*g));
    }
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);
}
static void paleteria_cursor_gl_forget_display(EGLDisplay display) {
    EGLContext contexts[8];size_t count=0;
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    for(size_t i=0;i<8;++i)if(paleteria_cursor_gl[i].context&&paleteria_cursor_gl[i].display==display)
        contexts[count++]=paleteria_cursor_gl[i].context;
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);
    for(size_t i=0;i<count;++i)paleteria_cursor_gl_forget(contexts[i]);
}
static void paleteria_cursor_gl_forget_surface(EGLSurface surface) {
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    for(size_t i=0;i<8;++i)if(paleteria_cursor_gl[i].surface==surface) {
        paleteria_cursor_gl[i].cached=false;paleteria_cursor_gl[i].surface=EGL_NO_SURFACE;
    }
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);
}
static PaleteriaCursorGl *paleteria_cursor_gl_get(void) {
    EGLContext context=eglGetCurrentContext();
    if(context==EGL_NO_CONTEXT)return NULL;
    PaleteriaCursorGl *g=NULL;
    for(size_t i=0;i<8;++i) {
        if(paleteria_cursor_gl[i].context==context)return paleteria_cursor_gl[i].program?&paleteria_cursor_gl[i]:NULL;
        if(!paleteria_cursor_gl[i].context)g=&paleteria_cursor_gl[i];
    }
    if(!g)return NULL;
    g->context=context;
    g->display=eglGetCurrentDisplay();
    const char *version=(const char*)glGetString(GL_VERSION);
    g->es3=version&&strstr(version,"OpenGL ES 3");
    const char *vs="attribute vec2 position; attribute vec4 color; uniform vec4 transform;"
        "uniform vec3 fill; varying vec3 rgb; varying vec2 uv; void main(){"
        "gl_Position=vec4(position*transform.xy+transform.zw,0.0,1.0);rgb=mix(color.rgb,fill,color.a);uv=color.xy;}";
    const char *fs="precision mediump float; varying vec3 rgb; varying vec2 uv;"
        "uniform sampler2D image; uniform bool background;"
        "void main(){gl_FragColor=background?texture2D(image,uv):vec4(rgb,1.0);}";
    GLuint vertex=paleteria_cursor_shader(GL_VERTEX_SHADER,vs),fragment=paleteria_cursor_shader(GL_FRAGMENT_SHADER,fs);
    if(vertex&&fragment) {
        g->program=glCreateProgram();glAttachShader(g->program,vertex);glAttachShader(g->program,fragment);
        glBindAttribLocation(g->program,0,"position");glBindAttribLocation(g->program,1,"color");
        glLinkProgram(g->program);GLint ok=0;glGetProgramiv(g->program,GL_LINK_STATUS,&ok);
        if(!ok){glDeleteProgram(g->program);g->program=0;APK64_LOGCW("CURSOR","Cursor shader link failed");}
    }
    if(vertex)glDeleteShader(vertex);
    if(fragment)glDeleteShader(fragment);
    if(!g->program)return NULL;
    g->transform=glGetUniformLocation(g->program,"transform");g->fill=glGetUniformLocation(g->program,"fill");
    g->background=glGetUniformLocation(g->program,"background");g->sampler=glGetUniformLocation(g->program,"image");
    /* Upload the outlined sprite once; movement changes only two uniforms. */
    GLfloat *mesh=malloc(20*16*6*6*sizeof(*mesh));size_t n=0;
    if(!mesh){glDeleteProgram(g->program);g->program=0;APK64_LOGCW("CURSOR","Cursor mesh allocation failed");return NULL;}
    static const int corner[6][2]={{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
    for(int row=0;row<20;++row)for(int col=0;col<16;) {
        char p=paleteria_cursor_pixels[row][col];int end=col+1;
        while(end<16&&paleteria_cursor_pixels[row][end]==p)++end;
        if(p!=' ')for(int v=0;v<6;++v) {
            mesh[n++]=(GLfloat)(corner[v][0]?end:col);mesh[n++]=(GLfloat)(row+corner[v][1]);
            mesh[n++]=p=='s'?.12f:.06f;mesh[n++]=p=='s'?.24f:.12f;mesh[n++]=p=='s'?.29f:.18f;
            mesh[n++]=p=='w'?1:0;
        }
        col=end;
    }
    g->vertices=(GLsizei)(n/6);glGenBuffers(1,&g->buffer);glBindBuffer(GL_ARRAY_BUFFER,g->buffer);
    glBufferData(GL_ARRAY_BUFFER,(GLsizeiptr)(n*sizeof(GLfloat)),mesh,GL_STATIC_DRAW);
    free(mesh);
    static const GLfloat quad[]={-1,-1,0,0,0,0, 1,-1,1,0,0,0, -1,1,0,1,0,0,
        -1,1,0,1,0,0, 1,-1,1,0,0,0, 1,1,1,1,0,0};
    glGenBuffers(1,&g->quad);glBindBuffer(GL_ARRAY_BUFFER,g->quad);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    if(g->es3)glGenVertexArrays(1,&g->vao);
    return g;
}
typedef struct PaleteriaCursorAttrib {
    GLint enabled,size,type,normalized,stride,buffer;
    void *pointer;
} PaleteriaCursorAttrib;
static void paleteria_cursor_attrib_save(GLuint index,PaleteriaCursorAttrib *a) {
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_ENABLED,&a->enabled);
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_SIZE,&a->size);
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_TYPE,&a->type);
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,&a->normalized);
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_STRIDE,&a->stride);
    glGetVertexAttribiv(index,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&a->buffer);
    glGetVertexAttribPointerv(index,GL_VERTEX_ATTRIB_ARRAY_POINTER,&a->pointer);
}
static void paleteria_cursor_render_locked(int width,int height,bool reverse,bool background,uint32_t cursor) {
    if((!(cursor&(1u<<30))&&!background)||width<=0||height<=0)return;
    const char *version=(const char*)glGetString(GL_VERSION);
    bool es3=version&&strstr(version,"OpenGL ES 3");
    GLint framebuffer,buffer,program,viewport[4],vao=0;GLboolean mask[4];
    GLenum target=es3?GL_DRAW_FRAMEBUFFER:GL_FRAMEBUFFER;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebuffer);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
    glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_VIEWPORT,viewport);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
    const GLenum caps[]={GL_BLEND,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_CULL_FACE,GL_SCISSOR_TEST,
        GL_SAMPLE_COVERAGE,GL_SAMPLE_ALPHA_TO_COVERAGE,GL_RASTERIZER_DISCARD};
    GLboolean enabled[8];int cap_count=es3?8:7;
    for(int i=0;i<cap_count;++i){enabled[i]=glIsEnabled(caps[i]);glDisable(caps[i]);}
    PaleteriaCursorAttrib attrib[2];
    if(es3)glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
    else for(GLuint i=0;i<2;++i)paleteria_cursor_attrib_save(i,&attrib[i]);
    PaleteriaCursorGl *g=paleteria_cursor_gl_get();
    if(g) {
        glBindFramebuffer(target,0);glViewport(0,0,width,height);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
        glUseProgram(g->program);if(es3)glBindVertexArray(g->vao);
        if(background&&g->cached) {
            GLint active,texture,sampler=0;
            glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
            if(es3){glGetIntegerv(GL_SAMPLER_BINDING,&sampler);glBindSampler(0,0);}
            glBindTexture(GL_TEXTURE_2D,g->texture);glUniform1i(g->sampler,0);glUniform1i(g->background,1);
            glUniform4f(g->transform,1,1,0,0);glBindBuffer(GL_ARRAY_BUFFER,g->quad);
            glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),(const void*)0);
            glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),(const void*)(2*sizeof(GLfloat)));
            glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glDrawArrays(GL_TRIANGLES,0,6);
            glBindTexture(GL_TEXTURE_2D,(GLuint)texture);if(es3)glBindSampler(0,(GLuint)sampler);glActiveTexture((GLenum)active);
        }
        if(cursor&(1u<<30)) {
        glUniform1i(g->background,0);glBindBuffer(GL_ARRAY_BUFFER,g->buffer);
        glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),(const void*)0);
        glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,6*sizeof(GLfloat),(const void*)(2*sizeof(GLfloat)));
        glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
        float x=(cursor&2047u)*(float)width/1280,y=((cursor>>11)&1023u)*(float)height/720;
        float scale=width>=1600?3:2,sign=reverse?-1:1;
        glUniform4f(g->transform,sign*scale*2/width,-sign*scale*2/height,sign*(2*x/width-1),sign*(1-2*y/height));
        if(cursor&(1u<<31))glUniform3f(g->fill,.25f,.88f,.82f);else glUniform3f(g->fill,.93f,1,1);
        glDrawArrays(GL_TRIANGLES,0,g->vertices);
        }
    }
    if(es3)glBindVertexArray((GLuint)vao);
    else for(GLuint i=0;i<2;++i) {
        PaleteriaCursorAttrib *a=&attrib[i];glBindBuffer(GL_ARRAY_BUFFER,(GLuint)a->buffer);
        glVertexAttribPointer(i,a->size,(GLenum)a->type,(GLboolean)a->normalized,a->stride,a->pointer);
        if(a->enabled)glEnableVertexAttribArray(i);else glDisableVertexAttribArray(i);
    }
    glBindBuffer(GL_ARRAY_BUFFER,(GLuint)buffer);glUseProgram((GLuint)program);
    glBindFramebuffer(target,(GLuint)framebuffer);glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);
    glColorMask(mask[0],mask[1],mask[2],mask[3]);for(int i=0;i<cap_count;++i)if(enabled[i])glEnable(caps[i]);
}
static void paleteria_cursor_draw(int width,int height,bool reverse) {
    uint32_t cursor=paleteria_cursor_snapshot();
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    paleteria_cursor_render_locked(width,height,reverse,false,cursor);
    EGLContext context=eglGetCurrentContext();
    for(size_t i=0;i<8;++i)if(paleteria_cursor_gl[i].context==context)paleteria_cursor_gl[i].last_cursor=cursor;
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);
}

static bool paleteria_cursor_copy_frame(PaleteriaCursorGl *g,GLuint source) {
    glBindFramebuffer(GL_FRAMEBUFFER,source);
    GLint samples=0;glGetIntegerv(GL_SAMPLE_BUFFERS,&samples);
    if(samples) {
        /* True GLES2 without framebuffer-blit cannot resolve MSAA. Leave the
         * game untouched and retain swap-driven cursor drawing in that case. */
        if(!g->es3)return false;
        if(!g->resolve_fbo)glGenFramebuffers(1,&g->resolve_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,g->resolve_fbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,g->texture,0);
        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)return false;
        GLboolean scissor=glIsEnabled(GL_SCISSOR_TEST);glDisable(GL_SCISSOR_TEST);
        glBlitFramebuffer(0,0,g->width,g->height,0,0,g->width,g->height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        if(scissor)glEnable(GL_SCISSOR_TEST);
    } else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,g->width,g->height);
    return true;
}
/* Preserve the clean guest frame before the pointer is composited into it. */
static void paleteria_cursor_capture(EGLDisplay display,EGLSurface surface,int width,int height,bool reverse) {
    if(width<=0||height<=0||eglGetCurrentContext()==EGL_NO_CONTEXT||
        eglGetCurrentDisplay()!=display||eglGetCurrentSurface(EGL_DRAW)!=surface)return;
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    GLint buffer,active,texture,framebuffer,read_framebuffer=0,unpack=0;
    bool es3=strstr((const char*)glGetString(GL_VERSION),"OpenGL ES 3")!=NULL;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);
    glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING,&framebuffer);
    if(es3)glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read_framebuffer);
    if(es3){glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack);glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);}
    PaleteriaCursorGl *g=paleteria_cursor_gl_get();
    if(g) {
        bool new_texture=!g->texture;
        if(new_texture)glGenTextures(1,&g->texture);
        glBindTexture(GL_TEXTURE_2D,g->texture);
        if(new_texture||g->width!=width||g->height!=height) {
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
        }
        g->width=width;g->height=height;g->display=display;g->surface=surface;g->reverse=reverse;
        g->cached=paleteria_cursor_copy_frame(g,0);
    }
    glBindFramebuffer(es3?GL_DRAW_FRAMEBUFFER:GL_FRAMEBUFFER,(GLuint)framebuffer);
    if(es3)glBindFramebuffer(GL_READ_FRAMEBUFFER,(GLuint)read_framebuffer);
    glBindTexture(GL_TEXTURE_2D,(GLuint)texture);glActiveTexture((GLenum)active);
    glBindBuffer(GL_ARRAY_BUFFER,(GLuint)buffer);if(es3)glBindBuffer(GL_PIXEL_UNPACK_BUFFER,(GLuint)unpack);
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);
}

/* Called on the Activity thread only. Never steal another thread's EGL context. */
static bool paleteria_cursor_refresh_idle(void) {
    EGLContext context=eglGetCurrentContext();
    if(context==EGL_NO_CONTEXT)return false;
    uint32_t cursor=paleteria_cursor_snapshot();bool presented=false;
    pthread_mutex_lock(&paleteria_cursor_gl_lock);
    for(size_t i=0;i<8;++i) {
        PaleteriaCursorGl *g=&paleteria_cursor_gl[i];
        if(g->context!=context||!g->cached||g->surface!=eglGetCurrentSurface(EGL_DRAW)||g->display!=eglGetCurrentDisplay())continue;
        /* Also redraw once when Minus hides the pointer on an idle menu. */
        if(cursor==g->last_cursor||!((cursor|g->last_cursor)&(1u<<30)))break;
        paleteria_cursor_render_locked(g->width,g->height,g->reverse,true,cursor);
        presented=eglSwapBuffers(g->display,g->surface)==EGL_TRUE;
        /* Keep our pointer out of the guest's next back buffer, including when
         * its EGL surface preserves contents or it draws only damaged areas. */
        paleteria_cursor_render_locked(g->width,g->height,g->reverse,true,0);
        if(presented)g->last_cursor=cursor;
        break;
    }
    pthread_mutex_unlock(&paleteria_cursor_gl_lock);return presented;
}
static void paleteria_cursor_draw_window(int surface_width,int surface_height,const Apk64DisplayPlan *plan) {
    /* Switch Mesa can report 0x0 for a working window. */
    paleteria_cursor_draw(plan?plan->physical_width:surface_width,plan?plan->physical_height:surface_height,
                          plan&&plan->aspect==APK64_DISPLAY_LANDSCAPE&&plan->reverse);
}
