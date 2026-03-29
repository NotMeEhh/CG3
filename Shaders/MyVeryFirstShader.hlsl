cbuffer CBPerObject : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 Color;
};

struct VS_IN
{
    float4 pos : POSITION;
    float4 color : COLOR;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    float4 worldPos = mul(input.pos, World);
    float4 viewPos = mul(worldPos, View);
    output.pos = mul(viewPos, Projection);
    output.color = Color; // use constant color
    return output;
}

float4 PSMain(PS_IN input) : SV_Target
{
    return input.color;
}