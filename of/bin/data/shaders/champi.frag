#pragma include "Common.hfrag"
#define COLOR_MODE 1
#define NUM_OCTAVES 4

uniform vec2 resolution;
uniform float time;


uniform float scale; // (3)
uniform float magnitude; // (.2)
uniform float speed; // (1)




float random (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}


float fbm ( in vec2 _st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));

    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_st);
        _st = rot * _st * 2.0 + shift;
        a *= 0.5;
    }
    
    return v;
}

float fbmS ( in vec2 _st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    // Rotate to reduce axial bias
    mat2 rot = mat2(cos(0.5), sin(0.5),
                    -sin(0.5), cos(0.50));

    //     for (int i = 0; i < NUM_OCTAVES; ++i) {
    //     v += a * noise(_st);
    //     _st = rot * _st * 2.0 + shift;
    //     a *= 0.5;
    // }
        v += a * noise(_st);
        _st = rot * _st * 2.0 + shift;
        a *= 0.5;
    
    
    return v;
}
void main() {
    vec2 st = NORMALIZE_UV(gl_TexCoord[0].xy)*scale;
    // st += st * abs(sin(u_time*0.1)*3.0);
    // vec3 color = vec3(0.0);

    vec2 q = vec2(0.);
    q.x = fbmS( st +time);
    // q.y = fbmS( st + time+ vec2(1.0) );
    // q.y = q.x;//fbm( st + vec2(1.0));

    vec2 r = vec2(0.);
    r.x = fbmS( st + 1.0*q + vec2(0.230,0.690)+ time );
    r.y = r.x*sin(r.x*10.0);
   // r.y = fbmS( st + 1.0*q + vec2(8.3,2.8)- time);
   // r*=5.0;
    float f = fbm(r);
    // f*=(f*f*f+.6*f*f+.5*f);
    f= f*2.0-1.0;
    
    #if COLOR_MODE==0
    vec2 targetSt = vec2(f,f*sin(f));
    targetSt*=magnitude*resolution;
    targetSt += NORMALIZE_UV(gl_TexCoord[0].xy);

    if(targetSt.x>1.0){targetSt.x=2.0-targetSt.x;}
    if(targetSt.y>1.0){targetSt.y=2.0-targetSt.y;}
    if(targetSt.x<0.0){targetSt.x=-targetSt.x;}
    if(targetSt.y<0.0){targetSt.y=-targetSt.y;}
    vec3 color = TEXTURE(tex0,NORMUV2TEXCOORD(targetSt)).rgb;
    #else
    vec3 modC = vec3(f,f*sin(f*6.0),f*cos(f*6.0));
    modC*=magnitude ;
    vec2 stt = NORMALIZE_UV(gl_TexCoord[0].xy);
    vec3 color = vec3(  TEXTURE(tex0,NORMUV2TEXCOORD(stt+modC.rg)).r,
                        TEXTURE(tex0,NORMUV2TEXCOORD(stt+modC.gb)).g,
                        TEXTURE(tex0,NORMUV2TEXCOORD(stt+modC.rb)).b);
    // color+=modC;
    #endif
    // color*=f;
    // color*=(f*f*f+.6*f*f+.5*f);
    //mix(vec3(0.101961,0.619608,0.666667),vec3(0.666667,0.666667,0.498039),clamp((f*f)*4.0,0.0,1.0));
    //  color = mix(color,vec3(0,0,0.164706),clamp(length(q),0.0,1.0));
    //color = mix(color,vec3(0.666667,1,1),clamp(length(r.x),0.0,1.0));

   gl_FragColor = vec4(color,1.);
     // gl_FragColor = vec4(color,1.);
}
