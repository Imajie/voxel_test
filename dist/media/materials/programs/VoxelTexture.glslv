float ATLAS_WIDTH = 4096.0;
float TEXTURE_WIDTH = 256.0;
float RATIO =  TEXTURE_WIDTH / ATLAS_WIDTH;
float NUM_TEXTURES_IN_ATLAS = (ATLAS_WIDTH / TEXTURE_WIDTH);
//vec4 _clipPosition1;

uniform mat4x4 worldMatrix;
uniform mat4x4 viewProj;

varying out vec4 textureAtlasOffset;
varying out vec3 worldPosition;

void main()
{
    
    float idx;
    float blocky;
    float blockx;
    //vec4 tmp = worldMatrix * gl_Vertex;
    worldPosition = gl_Vertex + vec4( 0.5, 0.5, 0.5, 0.5);

    idx = gl_Color.x * NUM_TEXTURES_IN_ATLAS;
    blocky = floor(idx/NUM_TEXTURES_IN_ATLAS);
    blockx = idx - blocky*NUM_TEXTURES_IN_ATLAS;
    textureAtlasOffset = vec4(blocky + 0.25, blockx + 0.25, 0.0, 0.0)*RATIO;

    gl_Position = ftransform();
} // main end
