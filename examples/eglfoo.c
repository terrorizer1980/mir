/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "./eglapp.h"
#include <assert.h>
#include <stdio.h>
#include <GLES2/gl2.h>

static GLuint LoadShader(const char *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (shader)
    {
        GLint compiled;
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            printf("LoadShader compile failed: %s\n", log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

/* Colours from http://design.ubuntu.com/brand/colour-palette */
#define MID_AUBERGINE 0.368627451f, 0.152941176f, 0.31372549f
#define ORANGE        0.866666667f, 0.282352941f, 0.141414141f

int main(int argc, char *argv[])
{
    const char vshadersrc[] =
        "attribute vec4 vPosition;            \n"
        "uniform float theta;                 \n"
        "varying vec2 texcoord;               \n"
        "void main()                          \n"
        "{                                    \n"
        "    float c = cos(theta);            \n"
        "    float s = sin(theta);            \n"
        "    mat2 m;                          \n"
        "    m[0] = vec2(c, s);               \n"
        "    m[1] = vec2(-s, c);              \n"
        "    vec2 p = m * vec2(vPosition);    \n"
        "    gl_Position = vec4(p, 0.0, 1.0); \n"
        "    texcoord = vec2(vPosition) * vec2(0.5) + vec2(0.5); \n"
        "}                                    \n";

    const char fshadersrc[] =
        "precision mediump float;                        \n"
        "uniform float theta;                            \n"
        "varying vec2 texcoord;                          \n"
        "uniform vec4 col;                               \n"
        "void main()                                     \n"
        "{                                               \n"
        "    float u = texcoord.x;         \n"
        "    float v = texcoord.y;         \n"
        "    float t = mod(theta, 6.283185308);   \n"
        "    float x = cos(17.4 * u) +  \n"
        "              cos(13.3 * t * u) + \n"
        "              cos(7.77 * v) + \n"
        "              cos(11.39 * t * v); \n"
        "    x /= 4.0;                                   \n"
        "    gl_FragColor = vec4(x, 0, 0.0, 1.0);        \n"
        "}                                               \n";

    const GLfloat vertices[] =
    {
        0.0f, 1.0f,
       -1.0f,-0.866f,
        1.0f,-0.866f,
    };
    GLuint vshader, fshader, prog;
    GLint linked, col, vpos, theta;
    int width = 512, height = 512;
    GLfloat angle = 0.0f;

    (void)argc;
    (void)argv;

    if (!mir_egl_app_init(&width, &height))
    {
        printf("Can't initialize EGL\n");
        return 1;
    }

    vshader = LoadShader(vshadersrc, GL_VERTEX_SHADER);
    assert(vshader);
    fshader = LoadShader(fshadersrc, GL_FRAGMENT_SHADER);
    assert(fshader);
    prog = glCreateProgram();
    assert(prog);
    glAttachShader(prog, vshader);
    glAttachShader(prog, fshader);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        printf("Link failed: %s\n", log);
        return 2;
    }

    glClearColor(MID_AUBERGINE, 1.0);
    glViewport(0, 0, width, height);

    glUseProgram(prog);

    vpos = glGetAttribLocation(prog, "vPosition");
    col = glGetUniformLocation(prog, "col");
    theta = glGetUniformLocation(prog, "theta");
    glUniform4f(col, ORANGE, 1.0f);

    glVertexAttribPointer(vpos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(0);

    while (1)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(theta, angle);
        angle += 0.005f;
        glDrawArrays(GL_TRIANGLES, 0, 3);
        mir_egl_swap_buffers();
    }

    return 0;
}
