#pragma include "Common.hfrag"

#define IMAGE_IS_MASK 1

#define INVERT_MASK 0




uniform SAMPLER_2D_TYPE maskTex;
uniform float tst;
uniform vec2 resolution;
uniform vec2 maskResolution;
uniform float maskThreshold; //(0.5)


uniform float smoothThresh; // (0.31)

const vec3 lumW = vec3(0.2126,0.7152,0.0722);
float getLuminance(vec3 c){return dot(c.rgb,lumW);}

vec3 maskAt(vec2 st){return TEXTURE(maskTex,st).rgb;}

vec3 maskIt(vec3 source,vec3 mask){
        float maskV = smoothstep(maskThreshold-smoothThresh,maskThreshold,
        #if INVERT_MASK
        1.0 -
        #endif
        getLuminance(mask.rgb));
        
        return source.rgb*maskV;
}

 void main()
 {

        vec2 st = ST();
        vec3 source = TEXTURE(tex0, st).rgb;
        vec3 img = maskAt(st);
        #if IMAGE_IS_MASK
        vec3 result = maskIt(source,img);
        #else
        vec3 result = maskIt(img,source);
        #endif

        FRAG_COLOR = vec4(result.rgb,1.0);

 }
