//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
float4 VS_TEST(float4 Pos : POSITION) : SV_POSITION
{
    return Pos;
}
// input : position �� (float4) 
// output : (float4) specification -> SV_position (system value :SV) 


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(float4 Pos : SV_POSITION) : SV_Target
{
    return float4(1.0f, 1.0f, 0.0f, 1.0f);    // Yellow, with Alpha = 1
}
// input : vertex shader�� output�� SV_POSITION 
// output : (float4) vector -> SV_TARGET���� output�ȴ� 

// hlsl : high level shader language
// google it SV_POSITION 