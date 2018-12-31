#version 330

#ifdef TARGET_OPENGLES
precision highp float;
#define IN attribute
#define OUT varying
#define TEXTURE texture2D
#else
#define IN in
#define OUT out
#define TEXTURE texture
#endif

IN vec4 position;
IN vec2 texcoord;
uniform mat4 modelViewProjectionMatrix;


// texture coordinates are sent to fragment shader
OUT vec2 v_texcoord;

void main()
{
    vec4 pos = modelViewProjectionMatrix * position;
    v_texcoord = texcoord;
    gl_Position = pos;
}
