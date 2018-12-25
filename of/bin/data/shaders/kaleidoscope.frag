#define USE_ARB 1

#define ALGO_NUM 0

#if USE_ARB
#extension GL_ARB_texture_rectangle : enable  
uniform sampler2DRect tex0;
#define TEXTURE(t,uv) texture2DRect(t,uv*resolution)
#else
uniform sampler2D tex0;
#define TEXTURE(t,uv) texture2D(t,uv*resolution)
#endif

uniform vec2 resolution;
uniform float zoom; //(1.0)
uniform vec2 zoomoffset; //(1.0)
uniform float rotation; // default (1)
uniform vec2 scale; // default(1,1)
uniform vec2 offset; // (0,0)
uniform vec2 mouse;
// uniform float time;

// vec2 kal2d(vec2 x,vec2 s,vec2 o){
//     vec2 hs = s/2.0;
//     return o+abs(0.5-mod(hs*(x-o) +.5,1.0))/hs;
// }

vec2 modul(vec2 x,vec2 s){
    return mod(s/2.0*(x-vec2(.5))+.5,1.);
}
vec2 triang(vec2 x,vec2 s){
    return (2.0*abs(modul(x+1.0/(2.0*s),s) - .5) -.5)/s + .5;
}

vec2 kal2d2(vec2 x,vec2 s,vec2 o,float z){
    return (triang(x-o,s) - 0.5) * z + 0.5 + o;
}

float mirrorOut(float v){
        float mod2 = mod(v,2.);
        if(mod2>1.){return 2.-(mod2);}
        return mod2;
}

float mirrorOutS(float v,float s){
        float mod2 = mod(v,2.*s);
        if(mod2>s){return 2.*s-(mod2);}
        return mod2;
}


vec2 zoom2d(vec2 uv,float zoom,vec2 center){ return (uv-center)*zoom + center;}


vec2 rot(vec2 v,float a,vec2 off){
    vec2 nx = vec2(cos(a),sin(a));
    vec2 ny = vec2(-nx.y,nx.x);
    vec2 i = v-off; 
    return vec2(dot(i,nx),dot(i,ny))+off;
}


void main( )
{
    // Normalized pixel coordinates (from 0 to 1)
    
    vec2 uv = gl_TexCoord[0].st/resolution.xy;
    
    //   // zoom
    // uv = zoom2d(uv,zoom,offset);  
  

#if ALGO_NUM==0
    //kaleidoscope it
    uv = kal2d2(uv.xy,scale,offset,zoom);
#else
    // different algo for kaleidoscoe
    uv.x = mirrorOutS(uv.x-offset.x,1.0/scale.x)+offset.x;
    uv.y = mirrorOutS(uv.y-offset.y,1.0/scale.y)+offset.y;
#endif
  
    // // rotate keeping aspect ratio
    float ary = resolution.y/resolution.x;
    uv.y*=ary;
    uv.xy = rot(uv.xy,rotation,vec2(0.5+zoomoffset.x+offset.x,(.5+zoomoffset.y+offset.y)*ary));
    uv.y/=ary;


// // compensate out of image by applying mirroring
    uv.x = mirrorOut(uv.x);
    uv.y = mirrorOut(uv.y);


    
   vec4 col = TEXTURE(tex0,uv.xy);
    
   // vec4 col = vec4(uv.y,0.0,0.0,1.0);
    // Output to screen
    gl_FragColor = col;
}