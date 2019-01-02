#define TARGET_RASPBERRY_PI 0 // will be changed by of

#if TARGET_RASPBERRY_PI
precision highp float;
#define IN attribute
#define OUT varying
#else
#define IN in
#define OUT out
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
