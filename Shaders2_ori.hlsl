// constantbuffer의 parameter를 가져오는 방법 
// cbuffer 라는 키워드, register(b0) : 첫 번째 buffer에 set하겠다 
// shader 내에서의 이름은 ConstatnBuffer가 된다 
// 만약 register(b1)할 때는 이름을 다른 것을 사용해야 한다 
cbuffer ConstantBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProjection;
}

struct VS_INPUT
{
    float4 Pos : POSITION0;
    float4 Color : COLOR0;
};

// VS_OUPUT 사용 가능 
struct PS_INPUT
{
    // SV_POSITION은 projection space에 정의되어 있지만 raterize r에 들어가면 자동으로 viewport transform에 의해 SS space 값으로 변하여 fragment shader input으로 들어가게 됨 
    float4 Pos : SV_POSITION; // semantics : 해당 type variable이 어떤 용도로 쓰이는 가 : string, 해당 variable의 용도에 대한 syntax로 사용됨 
    float4 Color: COLOR0;
};



//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS_TEST(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0; // 해당 struct의 variable을 0으로 초기화하겠다 
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    return output; // 하나의 input에 대해서 여러개의 output을 return 할 수 있음 
}
// 해당 shader는 여러 개의 input, output을 가질 수 있음 (fragment shader도 마찬가지)
// 삼각형에 대해서는 하나의 input만 받고 output으로 system position이 그대로 유지된 채 나가게 할 것임  
// float4 Pos : POSITION0 이랑 POSITION 이랑 같음 (doc) 



//--------------------------------------------------------------------------------------
// Pixel Shader 
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    return float4(input.Color.xyz, 1.0f);    // Yellow, with Alpha = 1
}
// return float4(input.PosTemp.xy + (float2)-.5, 1.0f, 1.0f); 
// 2d vector + 2d vector 가 된다 
// 그리고 기본적으로 3d 이여도 3d-razister에 homogeneous 값(1)을 쓰는 4d 으로 들어가기 때문임, 그래서 rasterizer의 output을 input으로 받는 PS의 input은 4d 이여야 겠죠???