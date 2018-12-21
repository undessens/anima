 #define USE_ARB 1


#if USE_ARB
#extension GL_ARB_texture_rectangle : enable  
uniform sampler2DRect tex0;
#define TEXTURE(t,uv) texture2DRect(t,uv*resolution)
#else
uniform sampler2D tex0;
#define TEXTURE(t,uv) texture2D(t,uv*resolution)
#endif


uniform sampler2D curveTex;
 uniform vec2 resolution;
 
 // uniform float shadows; // (100)
 // uniform float highlights; //(100)
 
 // uniform vec2 low; //(.2,.2)
 // uniform vec2 mid; // (.5,.5)
 // uniform vec2 high; // (.8,.8)
 // const vec2 start = vec2(0.0,0.0);
 // const vec2 end = vec2(1.0,1.0);

 // const float sq3 = 1.732050808;

 const vec3 lumW = vec3(0.2126,0.7152,0.0722);
 
// float compL4(float x,vec2 evalP,vec2 p1,vec2 p2,vec2 p3,vec2 p4){
//         return evalP.y*(x-p1.x)/(evalP.x-p1.x)*(x-p2.x)/(evalP.x-p2.x)*(x-p3.x)/(evalP.x-p3.x)*(x-p4.x)/(evalP.x-p4.x);
// }
// float pCurve(float x){
//         return  compL4(x,low,mid,high,start,end)+
//                 compL4(x,mid,high,start,end,low) +
//                 compL4(x,high,start,end,low,mid) +
//                 // compL4(x,start,end,low,mid) +
//                 compL4(x,end,low,mid,high,start) ;
// }

float pCurveTex(float x){
        return texture2D(curveTex,vec2(x,0.5)).r;
}

// float getLuminance(vec3 c){return length(c.rgb)/3.0;}
float getLuminance(vec3 c){return dot(c.rgb,lumW);}
 void main()
 {

        vec2 st = gl_TexCoord[0].st/resolution.xy;
        vec4 source = TEXTURE(tex0, st);
        float luminance = getLuminance(source.rgb);
        float targetLum = pCurveTex(luminance);
        vec3 deltaLum = vec3(targetLum-luminance);
        vec3 result = source.rgb+deltaLum;
        
        gl_FragColor = vec4(result.rgb, source.a);
        // gl_FragColor = vec4(vec3(luminance),source.a);
 }
