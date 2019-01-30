#pragma include "Common.hfrag"


uniform vec2 resolution;
uniform float size; // (2.0)








void main(void) {
        vec2 st = ST();
        
        #if USE_ARB
        float x = size ;
        #else
        float x= size/resolution.x;
        #endif

        st = (floor(st/vec2(size))+0.5)*vec2(size);
        vec4 originColor = TEXTURE( tex0, st   );

        
        
        FRAG_COLOR = originColor;

}
