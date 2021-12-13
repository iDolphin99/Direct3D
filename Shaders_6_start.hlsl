cbuffer TransformScene : register(b0)
{
    matrix g_mView;
    matrix g_mProjection;
}

cbuffer TransformModel : register(b1)
{
    // VS에서 사용하는 것 
    matrix g_mWorld; 

    // Phongshading을 위해 PS에서 사용하는 것 
    int g_mtAmbient, g_mtDiffuse, g_mtSpec; // 4 4 4 
    float g_shininess; // 4 => 16 // 
}


cbuffer LightBuffer : register(b2)
{
    float3 g_posLightCS;
    int g_lightColor;
    float3 g_dirLightCS;
    int g_lightFlag;
}

// register -> shader resource는 t(texture)를 씀, conventionally
// application level에서 shader에 Buffer t0에다가 Normal buffer resource를 가르키는 view를 set 해야 한다 
// 그렇게 함으로써 shader에서 NoramlBuffer에 indexing 할 수 있게 된다
Buffer<float3> NormalBuffer : register(t0);

// float3? unsigned char? 4channel인데 왜 float? 
// R8G8B8A8_UNORM -> UNORM 하면 0~1로 Normalize한 float을 쓰겠다는 의미, GPU에서 쓰는 정밀도가 8bit인 float으로 쓰겠다는 의미 
// 이렇게 하면 0->0, 255->1.0으로 mapping된 float으로 가져다 쓰는 것임 
// 만약 여기서 float2로 하면 강제적으로 R,G값만 사용할 수 있음, A값은 쓰지 않을거라서 float3해도 됨  
Texture2D<float4> EnvTexture : register(t1);

// Sampler 자체를 Resource로 만들어서 set해줘도 됨 -> 1: 04: 41에 있음 
SamplerState EnvSampler : register(s1)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Border;
    AddressV = Border;
    BorderColor = float4(0, 0, 0, 0);
};

struct VS_INPUT1
{
    float4 Pos : POSITION0;
    float4 Color : COLOR0;
    float3 Nor : NORMAL0;
};

struct VS_INPUT2
{
    float4 Pos : POSITION0;
    float3 Nor : NORMAL0;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT1
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float3 PosCS : POSITION0;
    float3 Nor : NORMAL;
};

struct PS_INPUT2
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
    float3 PosCS : POSITION0;
    float3 Nor : NORMAL;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
// position 정보만 들어오고 PS_INPUT1과 같게 두어서 현재 cube model이 쓰고 있는 PS와 같은 것을 쓰게 만들겠음 
PS_INPUT1 VS_P(float4 Pos : POSITION0)
{
    PS_INPUT1 output = (PS_INPUT1)0;    
    output.Pos = mul(Pos, g_mWorld);
    output.Pos = mul(output.Pos, g_mView);
    output.PosCS = output.Pos.xyz / output.Pos.w;

    output.Pos = mul(output.Pos, g_mProjection);
    output.Color = (float4)0; // 원인 issue를 처리함 
    output.Nor = (float4)0;

    return output;
}

PS_INPUT1 VS_PCN(VS_INPUT1 input)
{
    PS_INPUT1 output = (PS_INPUT1)0;
    output.Pos = mul(input.Pos, g_mWorld);
    output.Pos = mul(output.Pos, g_mView);
    output.PosCS = output.Pos.xyz / output.Pos.w;

    output.Pos = mul(output.Pos, g_mProjection);
    output.Color = input.Color;
    output.Nor = normalize(mul(input.Nor, mul(g_mView, g_mWorld)));

    return output;
}

PS_INPUT2 VS_PNT(VS_INPUT2 input)
{
    PS_INPUT2 output = (PS_INPUT2)0;
    output.Pos = mul(input.Pos, g_mWorld);
    output.Pos = mul(output.Pos, g_mView);
    output.PosCS = output.Pos.xyz / output.Pos.w;

    output.Pos = mul(output.Pos, g_mProjection);
    output.Tex = input.Tex;
    output.Nor = normalize(mul(input.Nor, mul(g_mView, g_mWorld)));
    return output;
}

float3 PhongLighting(float3 L, float3 N, float3 R, float3 V,
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess,
    float3 lightColor) {

    float dotNL = dot(N, L);
    //if (dot(N, L) <= 0) R = (float3)0;
    //float3 mtL = mtColor * lColor;
    // in CS, V is (0, 0, -1)
    // dot(R, V) equals to -R.z
    return mtcAmbient * lightColor + mtcDiffuse * lightColor * max(dotNL, 0) + mtcSpec * lightColor * pow(max(dot(R, V), 0), shininess);
}

float3 PhongLighting2(float3 L, float3 N, float3 R, float3 V,
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess,
    float3 lightColor) {

    float dotNL = dot(N, L);
    if (dot(N, L) <= 0) R = (float3)0;
    //float3 mtL = mtColor * lColor;
    // in CS, V is (0, 0, -1)
    // dot(R, V) equals to -R.z
    return mtcAmbient * lightColor + mtcDiffuse * lightColor * max(dotNL, 0) + mtcSpec * lightColor * pow(max(dot(R, V), 0), shininess);
}

float3 ConvertInt2Float3(int iColor) {
    float3 color;
    color.r = (iColor & 0xFF) / 255.f;
    color.g = ((iColor >> 8) & 0xFF) / 255.f;
    color.b = ((iColor >> 16) & 0xFF) / 255.f;
    return color;
}
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
// SV_PrimitiveID 는, System value Semantics는 Raterizer stage에서 주는 system value임, 그래서 VS input 값으로 쓸 수 없고 PS input으로 넣어줘야 함 
// 그렇다고 PS_INPUT에서 정의하면 되는게 아님, 그 strut도 VS_OUTPUT이기 때문 
// 그래서 해당 semantics를 다음과 같이 PS에 작성하여 사용함 
// shading을 하고 있는 PS는 shading을 CS에서 하고 있음 -> Space간의 missmatching 
// CS 정보를 기준으로 lighting이 이루어져야 하기 때문에 문제가 발생함 
float4 PS1(PS_INPUT1 input, uint primID : SV_PrimitiveID) : SV_Target
{
    // Light Color
    float3 lightColor = ConvertInt2Float3(g_lightColor);// float3(1.f, 1.f, 1.f);
    // Material Colors : ambient, diffuse, specular, shininess
    float3 mtcAmbient = ConvertInt2Float3(g_mtAmbient), mtcDiffuse = ConvertInt2Float3(g_mtDiffuse), mtcSpec = ConvertInt2Float3(g_mtSpec);
    float shine = g_shininess;
    // Light type (point)... 
    float3 L, N, R, V; // compute to do

    {
        if (g_lightFlag == 0)
            L = normalize(g_posLightCS - input.PosCS);
        else
            L = g_dirLightCS;
        V = normalize(-input.PosCS);

        // Vertex에 attribute로 갖고 있는 noraml 값을 Rasterizer에서 interpolation된 값 
        //N = normalize(input.Nor);
        // 0, 1 -> index 0, 1, 2 -> index 1 
        // 그러나 해당 noraml 은 OS의 점 -> Normal vector를 CS의 점으로 변환해줘야 함
        // 이렇게 수정함으로써 면(face)단위로 normal vector를 갖게 됨 
        // 이제 이게 됐기 때문에 stl file을 읽어서 모델을 추가하는 코드를 작성하자! 
        // C standard library에서 parsing을 함으로써 stl을 import하는 function을 직접 만들 수 있음 
        // 일단은 github에 stl_reader 관련 좋은 코드들이 있기 때문에 그걸 가져다가 쓰겠습니다
        N = normalize(mul(NormalBuffer[primID], mul(g_mView, g_mWorld)));
        // 근데 위에서 Normal을 이상하게 읽고 있음, 나누기2 
        // 육면체 cube를 그릴때는 두 개의 삼각형이 하나의 normal을 공유하는 데 stl model은 하나의 traiangle이 하나의 normal을 가지고 있음 -> 그러므로 나누기 2를 빼줘야 함 
        // 그럼 큐브는 따로 Shader를 만들어줄 수 있는데 우리는 application level에서 Normal을 맞춰 줄 예정 
 
        R = 2 * dot(N, L) * N - L;
    }

    float3 colorOut = PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    // Rasterizer stage에서 primitiveID를 fragment가 어떤 primitiveID에 속하는 지를 확인할 수 있게 하는 test code
    // 이 정보를 가지고 normal vector의 indexing하는 값으로 해당 ID를 사용할 수 있음 
    // 그럼 PS에서 해당 noraml buffer를 indexing 해야 함 
    //if (primID < 4 && primID >=2)
    //    return float4(1, 1, 1, 0);

    return float4(colorOut, 1);

    //float3 posCS = input.PosCS;
    //float3 unit_nor = normalize(input.Nor);
    //return float4(input.Color.xyz, 1.0f);
    //return float4((unit_nor + (float3)1.f) / 2.f, 1.0f);
}

float4 PS2(PS_INPUT2 input) : SV_Target
{
    //float3 colorOut = ConvertInt2Float3(g_lightColor);

    // u, v 값에 대해서 mapping된 것이 보여짐 
    //float3 colorOut = float3(input.Tex.xy, 0);

    // Sampler로 sampling 하는 방법 -> 해당 x,y값에 대해서 읽어와서 mapping함 
    // 우리가 보는 방향이 z 축이기 때문에 map의 하늘방향부터 보이는 것이 맞음 
    float3 colorOut = EnvTexture.Sample(EnvSampler, input.Tex.xy);

    return float4(colorOut, 1);
}