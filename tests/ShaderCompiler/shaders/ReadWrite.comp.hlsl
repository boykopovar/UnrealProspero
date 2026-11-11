[[vk::binding(0, 0)]] StructuredBuffer<uint> inputValues;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> outputValues;

[numthreads(64, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    outputValues[id.x] = inputValues[id.x] * 3 + 1;
}
