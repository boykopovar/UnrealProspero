struct Input
{
    [[vk::location(0)]] float2 Position : ATTRIBUTE0;
    [[vk::location(1)]] int2 Offset : ATTRIBUTE1;
    [[vk::location(2)]] uint4 Tags : ATTRIBUTE2;
};

float4 main(Input input) : SV_Position
{
    return float4(input.Position + float2(input.Offset), float(input.Tags.x + input.Tags.y), float(input.Tags.z + input.Tags.w));
}
