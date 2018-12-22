 #define USE_ARB 1
#define MULTI_CHANNEL 1

#if USE_ARB
#extension GL_ARB_texture_rectangle : enable  
uniform sampler2DRect tex0;
#define TEXTURE(t,uv) texture2DRect(t,uv)
#else
uniform sampler2D tex0;
#define TEXTURE(t,uv) texture2D(t,uv)
#endif


uniform sampler2D curveTex;
 uniform vec2 resolution;


 const vec3 lumW = vec3(0.2126,0.7152,0.0722);
float getLuminance(vec3 c){return dot(c.rgb,lumW);}

vec4 pCurveTex(float x){return texture2D(curveTex,vec2(x,0.5));}

// float getLuminance(vec3 c){return length(c.rgb)/3.0;}


 void main()
 {

        vec2 st = gl_TexCoord[0].st;
        vec4 source = TEXTURE(tex0, st);
        #if MULTI_CHANNEL
        source.r*=pCurveTex(source.r).r/source.r;
        source.g*=pCurveTex(source.g).g/source.g;
        source.b*=pCurveTex(source.b).b/source.b;
        #endif
        float luminance = getLuminance(source.rgb);
        float targetLum = pCurveTex(luminance).a;
        vec3 deltaLum = vec3(targetLum-luminance);
        vec3 result = source.rgb+deltaLum;
        
        gl_FragColor = vec4(result.rgb, source.a);
        // gl_FragColor = vec4(vec3(luminance),source.a);
 }
