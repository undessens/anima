#pragma include "Common.hfrag"

uniform vec2 resolution;


 void main()
 {
        vec3 origin = TEXTURE(tex0, ST()).rgb;
        origin.r = origin.g+origin.r;
        origin.b = origin.g+origin.b;
        FRAG_COLOR = vec4(origin,1.0);

 }
