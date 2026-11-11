[[vk::binding(0, 0)]] RWStructuredBuffer<uint> outputValues;
[[vk::binding(0, 1)]] cbuffer Constants
{
    uint multiplier;
    uint addend;
};

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputValues[id.x] = id.x * multiplier + addend;
}
