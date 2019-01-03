#pragma include "Common.hfrag"

#define ALGO_NUM 0
#define TRI_MODE 3
#define MIRROR_OUT 0

#define DO_SKEW 0// TRI_MODE==3



#define PI 3.1415
uniform vec2 resolution;
uniform float zoom; //(1.0)
uniform vec2 zoomoffset; //(1.0)
uniform float rotation; // default (0)
uniform vec2 scale; // default(1,1)
uniform vec2 offset; // (0,0)
// uniform vec2 mouse;
// uniform float time;
uniform float dbg; // (0)
uniform float dbg2; // (0)
uniform float dbg3; // (0)

const float sqrt2 = 1.414213562;
const float isqrt2 = 1.0/sqrt2;


vec2 symDiag(vec2 res){
    if(res.y>res.x){return vec2(res.y,res.x);}
    return res;
}
vec2 symPerp(vec2 res){
    if(abs(res.y+res.x)>1.0){return 1.0 - abs(vec2(res.y,res.x));}
    return res;
}

float mirrorOut(float v){
    float mod2 = mod(v,2.);
    if(mod2>1.){return 2.-(mod2);}
    return mod2;
}


vec2 mirrorOutSV(vec2 v,vec2 s){
    vec2 mod2 = mod(v,2.*s);
    if(mod2.x>s.x){mod2.x =  2.*s.x-mod2.x;}
    if(mod2.y>s.y){mod2.y =  2.*s.y-mod2.y;}
    return mod2;
}

vec2 mirrorOutTSV(vec2 v,vec2 s){
    vec2 mod2 = mod(v,s);
    if(mod2.x+mod2.y>(s.x+s.y)/2.0){
        mod2 =  s-mod2;
    }
    // if(mod2.y>s.y){mod2.y =  2.*s.y-mod2.y;}
    return mod2;
}
vec2 repeatV(vec2 v,vec2 s,vec2 skew){
    float vy = mirrorOut(v.y/s.y)*s.y;
    return vec2(mod(v.x+vy*skew.x,s.x),vy);
}
vec2 zoom2d(vec2 uv,float zoom,vec2 center){ return (uv-center)*zoom + center;}


vec2 rot(vec2 v,float a,vec2 off){
    vec2 nx = vec2(cos(a),sin(a));
    vec2 ny = vec2(-nx.y,nx.x);
    vec2 i = v-off; 
    return vec2(dot(i,nx),dot(i,ny))+off;
}

vec2 proj(vec2 inp,vec2 d1,vec2 d2){
    return vec2(dot(inp,d1),dot(inp,d2));
}

vec2 triK(vec2 v,float off){
    float skewF = 1.;
    v/=vec2(1.0/3.0,1.0);
    float numC = mod(v.x,3.0);
    v.x = mod(v.x,1.0);

    if(numC<1.0){ // 0
        // if(v.x>v.y){v.y=v.x + (v.y-v.x)*skewF;}
        v = -symDiag(-v);
    }
    else if(numC<2.0){ // 1
        // if(v.x<v.y){v.y=v.x + (v.y-v.x)*skewF;}
        v = -symDiag(-v);

        vec2 ax1 = rot(vec2(1.0,.0),0.0,vec2(.0,0.0));
        vec2 ax2 = rot(vec2(.0,sqrt2),-PI/4.0,vec2(0.0));
        v =proj(v,ax1,ax2) ;
        v = rot(v,PI/2.0,vec2(0.50));
        


    }
    else{ // 2
        
        
        v = -symDiag(-v);

                vec2 ax1 = rot(vec2(1.,.0),-PI/2.0,vec2(.0,0.0));
        vec2 ax2 = rot(vec2(.0,sqrt2),-PI/4.0,vec2(0.0));
        v =proj(v,ax1,ax2) ;
        // v.y*=dbg3;
        v = rot(v,PI,vec2(0.50));
        

    }

    v*=vec2(1.0/3.0,1.0);
    return v;
}

vec2 kal2d3(vec2 x,vec2 scale,vec2 o,float z,float ar){

    // scale.x*=ar;
    #if  TRI_MODE==3
    // vec2 anchor = vec2(1.0/6.0,0.5);
    vec2 anchor = vec2(0.);//*(1.0+z);
    // o.x-=1.0/3.0;
    scale.x*=3.0;
    #else
    vec2 anchor = vec2(0.5);
    #endif
    vec2 offs = o+(anchor)*(1.0-scale);//+0.5;//-vec2(0.5);

    
    #if  TRI_MODE==3
    float skew = scale.x/6.0/scale.y;
    vec2 skewInc = vec2(skew,0.0);
    // offs-=skewInc;
    vec2 res = repeatV((x-offs) ,scale,skewInc)+offs ;
    #else
    
    vec2 res = mirrorOutSV((x-offs) ,scale)+offs ;
    #endif
    
    vec2 whRec = scale;
    vec2 resZ = (res-offs)/whRec;
    
    #if TRI_MODE==1 
    resZ = symDiag(resZ);


    #elif TRI_MODE==2
    resZ = symDiag(vec2(resZ.x*2.0,resZ.y));
    resZ.x/=2.0;
    if(resZ.x>0.5){
        resZ.x-=1.0 ;
        resZ.x*=-2.0;
        resZ = symDiag(resZ);
        resZ.x/=-2.0;
        resZ.x+=1.0 ;
    }


    #elif  TRI_MODE==3
    resZ = triK(resZ,floor((res.y - offs.y )/scale.y));

    #endif
    


    resZ =   (resZ- anchor)*z+anchor;
    res = (resZ)*whRec+offs;

    return res;

}





void main( )
{
    // Normalized pixel coordinates (from 0 to 1)
    
    vec2 uv = NORMALIZE_UV(ST());
    


    //kaleidoscope it
    float ar = resolution.x/resolution.y;
    vec2 iscale = 1.0/scale;
    uv = kal2d3(uv.xy,iscale,offset,zoom,ar);


    // // rotate keeping aspect ratio
    float ary = resolution.y/resolution.x;
    uv.y*=ary;
    uv.xy = rot(uv.xy,rotation,vec2(0.5+zoomoffset.x+offset.x,(.5+zoomoffset.y+offset.y)*ary));
    uv.y/=ary;

    #if MIRROR_OUT
// // compensate out of image by applying mirroring
uv.x = mirrorOut(uv.x);
uv.y = mirrorOut(uv.y);
#else
uv = clamp(uv,vec2(0.0),vec2(1.0));
#endif


vec4 col = TEXTURE(tex0,NORMUV2TEXCOORD(uv.xy));

   // vec4 col = vec4(uv.xy,0.0,1.0);
    // Output to screen
    FRAG_COLOR = col;
}
