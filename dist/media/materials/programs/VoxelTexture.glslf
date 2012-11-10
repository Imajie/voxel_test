float ATLAS_WIDTH = 4096.0;
float TEXTURE_WIDTH = 256.0;
float RATIO =  TEXTURE_WIDTH / ATLAS_WIDTH;

uniform sampler2D Atlas;
varying in vec4 textureAtlasOffset;
varying in vec3 worldPosition;

//varying in vec3 normal;
void main()
{
    
    vec2 uv;
    vec3 wp = worldPosition;
    // ddx then ddy gives back positive normals for all the positive faces
    vec3   worldNormal = cross(dFdy(wp.xyz), dFdx(wp.xyz));
    worldNormal = normalize(worldNormal);
    uv = vec2( 1.0, 1.0);
    if (worldNormal.x > 0.5)
        uv = fract(vec2(-wp.z, -wp.y));
    if (worldNormal.x < -0.5)
        uv = fract(vec2(-wp.z, wp.y));
    if (worldNormal.y > 0.5)
        uv = fract(wp.xz);
    if (worldNormal.y < -0.5)
        uv = fract(vec2(-wp.x, wp.z));
    if (worldNormal.z > 0.5)
        uv = fract(vec2(wp.x, -wp.y));
    if (worldNormal.z < -0.5)
        uv = fract(vec2(-wp.x, -wp.y));

    uv = uv*0.5;
    vec4 voff = textureAtlasOffset + vec4(uv.x, uv.y, 0.0,0.0)*RATIO;
    gl_FragColor = texture2D(Atlas, voff.xy);
    
    return;
} 
