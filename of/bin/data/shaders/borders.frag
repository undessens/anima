
#define USE_ARB 1
#define INVERT 1
#define MODE 1
#define TRACK_MODE 1

#if USE_ARB
#extension GL_ARB_texture_rectangle : enable  
uniform sampler2DRect tex0;
#define TEXTURE(t,uv) texture2DRect(t,uv)
#else
uniform sampler2D tex0;
#define TEXTURE(t,uv) texture2D(t,uv)
#endif


uniform float size; // (2.0)
uniform float minEdge; // (.0)
uniform float maxEdge; // (1.0)
uniform float smoothEdge; // (1.0)



void main(void) {
        vec2 st = gl_TexCoord[0].xy;
        float x = size ;
        float y = size ;
        vec3 topL = TEXTURE( tex0, vec2( st.x - x, st.y - y ) ).rgb;
        vec3 midL = TEXTURE( tex0, vec2( st.x - x, st.y     ) ).rgb;
        vec3 botL = TEXTURE( tex0, vec2( st.x - x, st.y + y ) ).rgb;
        vec3 topR = TEXTURE( tex0, vec2( st.x + x, st.y - y ) ).rgb;
        vec3 midR = TEXTURE( tex0, vec2( st.x + x, st.y     ) ).rgb;
        vec3 botR = TEXTURE( tex0, vec2( st.x + x, st.y + y ) ).rgb;
        vec3 topMid = TEXTURE( tex0, vec2( st.x , st.y - y ) ).rgb;
        vec3 botMid = TEXTURE( tex0, vec2( st.x , st.y + y ) ).rgb;
        vec3 horizEdge = -topL-botL - 2.0*midL   + topR+botR+2.0*midR;
        vec3 vertEdge  = -topL-topR - 2.0*topMid + botR+botL+2.0*botMid;
        
        float edge = length(sqrt((horizEdge.rgb * horizEdge.rgb) + (vertEdge.rgb * vertEdge.rgb)));
        #if TRACK_MODE==0
        edge = (edge-minEdge)/(maxEdge-minEdge)+minEdge ;
        #else
        edge = step(edge,minEdge)- step(edge,maxEdge);
        #endif
        edge = clamp(edge,0.,1.0);
        #if INVERT
        edge = 1.0-edge;
        #endif


        vec4 originColor = TEXTURE( tex0, st );
        #if MODE==1 // blak and white sketch
        originColor.rgb = vec3(edge);
        #else
        originColor.rgb *= smoothstep(0.5-smoothEdge,0.5+smoothEdge,edge);
        #endif
        
        gl_FragColor = originColor;

}