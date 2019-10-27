#pragma include "Common.hfrag"


uniform vec2 resolution;
uniform float size; // (2.0)


#define MODE 0
#define COMPLEXITY 2




// no multiline defines on gles rpi...
#define PASS(sx,sy) (TEXTURE( tex0, vec2( st.x - sx, st.y - sy ) ).rgb + TEXTURE( tex0, vec2( st.x - sx, st.y     ) ).rgb*2.0 +  TEXTURE( tex0, vec2( st.x - sx, st.y + sy ) ).rgb +  TEXTURE( tex0, vec2( st.x + sx, st.y - sy ) ).rgb +  TEXTURE( tex0, vec2( st.x + sx, st.y     ) ).rgb*2.0 +  TEXTURE( tex0, vec2( st.x + sx, st.y + sy ) ).rgb +  TEXTURE( tex0, vec2( st.x    , st.y - sy ) ).rgb*2.0 +  TEXTURE( tex0, vec2( st.x    , st.y + sy ) ).rgb*2.0)/8.0   



void main(void) {
        vec2 st = ST();
        
        #if USE_ARB
        float x = size ;
        float y = x ;
        #else
        float x= size/resolution.x;
        float y= size/resolution.y;
        #endif


        vec4 originColor = TEXTURE( tex0, st   );

        #if COMPLEXITY==1
        originColor.rgb = ( originColor.rgb + .7*PASS(x,y))/2.0;
        #elif COMPLEXITY==2
        originColor.rgb= (originColor.rgb  + .7/2.0*(PASS(x,y)+ PASS(x/2.0,y/2.0))) / 2.0;
        #elif COMPLEXITY==3
        originColor.rgb= (originColor.rgb  + .7/4.0*(PASS(x,y)+ PASS(x/2.0,y/2.0)+ PASS(x/4.0,y/4.0) + PASS(x*3.0/4.0,y*3.0/4.0))) / 2.0;
        #endif

        
        FRAG_COLOR = originColor;

}
