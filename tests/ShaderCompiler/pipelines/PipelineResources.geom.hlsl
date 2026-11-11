cbuffer GeometryConstants : register(b0)
{
    float4 Offset;
};

struct Vertex
{
    float4 Position : SV_Position;
};

[maxvertexcount(3)]
void main(triangle Vertex input[3], inout TriangleStream<Vertex> output)
{
    for (uint index = 0; index < 3; ++index)
    {
        Vertex vertex = input[index];
        vertex.Position.xy += Offset.xy;
        output.Append(vertex);
    }
    output.RestartStrip();
}
