 #define USE_ARB 1


#if USE_ARB
#extension GL_ARB_texture_rectangle : enable  
uniform sampler2DRect tex0;
#define TEXTURE(t,uv) texture2DRect(t,uv)
#else
uniform sampler2D tex0;
#define TEXTURE(t,uv) texture2D(t,uv)
#endif



uniform sampler2DRect maskTex;
uniform float tst;
uniform vec2 resolution;
uniform vec2 maskResolution;
uniform float maskThreshold; //(0.5)
uniform float invertMask; // (0)
uniform float imageIsMask; // (1)
uniform float smoothThresh; // (0.31)

const vec3 lumW = vec3(0.2126,0.7152,0.0722);
float getLuminance(vec3 c){return dot(c.rgb,lumW);}

vec3 maskAt(vec2 st){return texture2DRect(maskTex,st/resolution*maskResolution).rgb;}

vec3 maskIt(vec3 source,vec3 mask){
        float maskV = smoothstep(maskThreshold-smoothThresh,maskThreshold+smoothThresh,getLuminance(mask.rgb));
        if(invertMask>0.0){maskV = 1.0-maskV;}
        return source.rgb*maskV;
}

 void main()
 {

        vec2 st = gl_TexCoord[0].st;
        vec3 source = TEXTURE(tex0, st).rgb;
        vec3 img = maskAt(st);
        vec3 result = imageIsMask>0.0?maskIt(source,img):maskIt(img,source);
        // if(){result = ;}
        // else{result = ;}
        // gl_FragColor = vec4(img.rgb, 1.0);
        // vec2 ratio = st*maskResolution/resolution;
        gl_FragColor = vec4(result.rgb,1.0);

 }
