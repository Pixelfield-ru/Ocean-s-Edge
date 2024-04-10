#sg_pragma once

#define BLOCKS_ATLAS_SIZE vec2(960, 960)

vec2 currentUVs[4] = vec2[] (
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0)
);

// BLOCKS TYPES
#define OE_BT_MUD_WITH_GRASS 1

// FACES
#define FACE_UP 0
#define FACE_BOTTOM 1
#define FACE_RIGHT 2
#define FACE_LEFT 3
#define FACE_FACE 4
#define FACE_BACK 5

void changeUVForCurrentVertex(int blockType, int blockFace)
{
    if(blockType == OE_BT_MUD_WITH_GRASS && blockFace == FACE_UP)
    {
        currentUVs[0] = vec2(0, 0);
        currentUVs[1] = vec2(0, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[2] = vec2(160 / BLOCKS_ATLAS_SIZE.x, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[3] = vec2(160 / BLOCKS_ATLAS_SIZE.x, 0);
    }
    else if(blockType == OE_BT_MUD_WITH_GRASS && blockFace == FACE_BOTTOM)
    {
        currentUVs[0] = vec2(480 / BLOCKS_ATLAS_SIZE.x, 0);
        currentUVs[1] = vec2(480 / BLOCKS_ATLAS_SIZE.x, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[2] = vec2(640 / BLOCKS_ATLAS_SIZE.x, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[3] = vec2(640 / BLOCKS_ATLAS_SIZE.x, 0);
    }
    else if(blockType == OE_BT_MUD_WITH_GRASS && (blockFace == FACE_LEFT || blockFace == FACE_RIGHT || blockFace == FACE_FACE || blockFace == FACE_BACK))
    {
        currentUVs[0] = vec2(160 / BLOCKS_ATLAS_SIZE.x, 320 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[1] = vec2(160 / BLOCKS_ATLAS_SIZE.x, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[2] = vec2(320 / BLOCKS_ATLAS_SIZE.x, 160 / BLOCKS_ATLAS_SIZE.y);
        currentUVs[3] = vec2(320 / BLOCKS_ATLAS_SIZE.x, 320 / BLOCKS_ATLAS_SIZE.y);
    }
}
