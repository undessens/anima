#pragma include "Common.hfrag"

uniform vec2 resolution;

#define USE_HUE 1
#define USE_SAT 1
#define USE_VAL 1


uniform float hueRes; // (5)
uniform float hueOff; // (0)
uniform float hueSmooth; // (0.1)

uniform float satRes; // (5)
uniform float satOff; // (0)
uniform float satSmooth; // (0.1)

uniform float valRes; // (5)
uniform float valOff; // (0)
uniform float valSmooth; // (0.1)


vec3 rgb2hsv(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// float getStepped(float x,float res,float off,float vsmooth){
//                 float floated =x*res+off;
//                 float floored = (floor(floated));
//                 return (floored-1.0+ smoothstep(0.0,vsmooth,fract(floated)))/res;
// }   

#define getStepped( x, res, off, vsmooth) ((floor(x*res+off)-1.0+ smoothstep(0.0,vsmooth,fract(x*res+off)))/res)

void main(void) {
        vec2 st = ST();
        vec4 originColor = TEXTURE( tex0, st );
        vec3 hsv = rgb2hsv(originColor.rgb);
        #if USE_HUE
        hsv.x = getStepped(hsv.x,hueRes,hueOff,hueSmooth);
        #endif
        #if USE_SAT
        hsv.y = getStepped(hsv.y,satRes,satOff,satSmooth);
        #endif
        #if USE_VAL
        hsv.z = getStepped(hsv.z,valRes,valOff,valSmooth);
        #endif

        originColor.rgb = hsv2rgb(hsv);
        FRAG_COLOR = originColor;

}
