[[vk::binding(0, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputValues[id.x] = id.x * 3 + 1;
}
