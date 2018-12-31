#pragma include "Common.hfrag"

uniform vec2 resolution;
uniform float vertical; // (1)
uniform float switchSide; // (1)
// uniform float zoom; //(1.0)
// uniform float zoomoffset; //(1.0)
// uniform float rotation; // default (1)
uniform float offset; // (0.5)
// uniform vec2 mouse;
// uniform float time;



float simpleMirror(float x,float o,bool sw){
    if((x<o) != sw) {return x;}
    return 2.0*o-x;

}

float mirrorOut(float v){
        float mod2 = mod(v,2.);
        if(mod2>1.){return 2.-(mod2);}
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
    
    vec2 uv = NORMALIZE_UV(gl_TexCoord[0].st);
    
    //   // zoom
    // uv = zoom2d(uv,zoom,offset);  
  
    if(vertical>0.0){
        uv.y = simpleMirror(uv.y,offset,switchSide>0.0);
    }
    else{
        uv.x = simpleMirror(uv.x,offset,switchSide>0.0);
    }

  
    // // rotate keeping aspect ratio
    // float ary = resolution.y/resolution.x;
    // uv.y*=ary;
    // uv.xy = rot(uv.xy,rotation,vec2(0.5+zoomoffset.x+offset.x,(.5+zoomoffset.y+offset.y)*ary));
    // uv.y/=ary;


// // compensate out of image by applying mirroring
    uv.x = mirrorOut(uv.x);
    uv.y = mirrorOut(uv.y);


    
   vec4 col = TEXTURE(tex0,NORMUV2TEXCOORD(uv));
   //  vec2 cuv = gl_TexCoord[0].st;
   // vec4 col = vec4(cuv,0.0,1.0);
    // Output to screen
    gl_FragColor = col;
}