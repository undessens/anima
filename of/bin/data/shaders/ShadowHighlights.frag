#pragma include "Common.hfrag"

#define MULTI_CHANNEL 0
#define BASE_CHANNEL 0


uniform sampler2D curveTex;
uniform vec2 resolution;
uniform vec3 globalCol; //(1.0)


 const vec3 lumW = vec3(0.2126,0.7152,0.0722);
// float getLuminance(vec3 c){return dot(c.rgb,lumW);}
#define getLuminance(c) dot(c.rgb,lumW)

// vec4 pCurveTex(float x){return TEXTURE(curveTex,vec2(x,0.5));}
#define pCurveTex(x) TEXTURE(curveTex,vec2(x,0.5))
// float getLuminance(vec3 c){return length(c.rgb)/3.0;}


 void main()
 {

        vec2 st = ST();
        vec4 source = TEXTURE(tex0, st);
        #if MULTI_CHANNEL
        source.r=pCurveTex(source.r).r;
        source.g=pCurveTex(source.g).g;
        source.b=pCurveTex(source.b).b;
        #endif
        #if BASE_CHANNEL
        float luminance = getLuminance(source.rgb);
        float targetLum = pCurveTex(luminance).a;
        vec3 deltaLum = vec3(targetLum-luminance);
        FRAG_COLOR = vec4((source.rgb+deltaLum)*globalCol, source.a);
        #else
        FRAG_COLOR = vec4(source.rgb*globalCol, source.a);
        #endif
        
        
        
 }
