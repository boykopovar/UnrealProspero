[[vk::binding(0, 0)]] Texture3D<float> inputTexture;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint level = id.x % 4;
    uint width = 8u >> level;
    uint voxel = id.x / 4;
    uint z = (voxel * 3 + 1) % width;
    outputValues[id.x] = uint(inputTexture.Load(int4(voxel % width, (voxel / width) % width, z, level)));
}
