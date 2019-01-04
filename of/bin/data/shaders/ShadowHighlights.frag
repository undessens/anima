#pragma include "Common.hfrag"

#define MULTI_CHANNEL 0


uniform sampler2D curveTex;
 uniform vec2 resolution;


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
        float luminance = getLuminance(source.rgb);
        float targetLum = pCurveTex(luminance).a;
        vec3 deltaLum = vec3(targetLum-luminance);
        vec3 result = source.rgb+deltaLum;
        
        FRAG_COLOR = vec4(result.rgb, source.a);
        
 }
