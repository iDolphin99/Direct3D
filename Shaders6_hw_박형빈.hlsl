/***************** Constant Buffer ********************/
cbuffer TransformScene : register(b0)
{
    matrix g_mView;
    matrix g_mProjection;
}

cbuffer TransformModel : register(b1)
{
    matrix g_mWorld; 

    int g_mtAmbient, g_mtDiffuse, g_mtSpec; 
    float g_shininess; 
}

cbuffer LightBuffer : register(b2)
{
    float3 g_posLightCS;
    int g_lightColor;
    float3 g_dirLightCS;
    int g_lightFlag;

    matrix g_mView2EnvOS;
}

/***************** Shader Resource ********************/
Buffer<float3> NormalBuffer : register(t0);
Texture2D<float4> EnvTexture : register(t1);

/********************* Sampler ************************/
SamplerState EnvSamplerCpp : register(s0);

/********************* struct ************************/
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
// 
// position -> PS_INPUT1
PS_INPUT1 VS_P(float4 Pos : POSITION0)
{
    PS_INPUT1 output = (PS_INPUT1)0;    
    output.Pos = mul(Pos, g_mWorld);
    output.Pos = mul(output.Pos, g_mView);
    output.PosCS = output.Pos.xyz / output.Pos.w;

    output.Pos = mul(output.Pos, g_mProjection);
    output.Color = (float4)0; // 원인 issue 제거 
    output.Nor = (float4)0;

    return output;
}

// position, color, normal -> PS_INPUT1
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

// position, normal, texture -> PS_INPUT2
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


/********************* PhongLighting ************************/
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
// phonglighting Pixel shader 
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
        //N = normalize(input.Nor);
        N = normalize(mul(NormalBuffer[primID], mul(g_mView, g_mWorld))); 

        R = 2 * dot(N, L) * N - L;
    }

    float3 colorOut = PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    //if (primID < 4 && primID >=2)
    //    return float4(1, 1, 1, 0);

    return float4(colorOut, 1);

    //float3 posCS = input.PosCS;
    //float3 unit_nor = normalize(input.Nor);
    //return float4(input.Color.xyz, 1.0f);
    //return float4((unit_nor + (float3)1.f) / 2.f, 1.0f);
}

// phonglighting Sphere Pixel shader 
float4 PS2(PS_INPUT2 input) : SV_Target
{
    float3 colorOut = ConvertInt2Float3(g_lightColor);

    return float4(colorOut, 1);
}

// Envmap Sphere Pixel shader 
float4 PS3(PS_INPUT2 input) : SV_Target
{
    //float3 colorOut = ConvertInt2Float3(g_lightColor);
    //float3 colorOut = float3(input.Tex.xy, 0);
    float3 colorOut = EnvTexture.Sample(EnvSamplerCpp, input.Tex.xy);

    return float4(colorOut, 1);
}

// Reflection Pixel shader 
float4 PS4(PS_INPUT1 input, uint primID : SV_PrimitiveID) : SV_Target
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

        //N = normalize(input.Nor);
        // 0, 1 ==> 0
        // 1, 2 ==> 1
        N = normalize(mul(NormalBuffer[primID], mul(g_mView, g_mWorld))); 

        R = 2 * dot(N, L) * N - L;
    }

    // Reflection vector
    float3 vR = 2 * dot(N, V) * N - V;
    vR = normalize(vR);

    float3 color1 = PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    float3 envR = mul(vR, g_mView2EnvOS);
    envR = normalize(envR);

    float X_PI = 3.14f;
    float phi = asin(envR.z); // 90~-90
    float cosphi = cos(phi);
    float theta = acos(max(min(envR.x / cos(phi),0.999f),-0.999f)); // 0 ~ pi
    float u = envR.y >= 0 ? theta / (2 * X_PI) : (2 * X_PI - theta) / (2 * X_PI);
    float v = (X_PI / 2 - phi) / X_PI;

    //float3 texColor = EnvTexture.Sample(EnvSamplerCpp, input.Tex.xy);
    float3 texColor = EnvTexture.Sample(EnvSamplerCpp, float2(u, v));

    R = normalize(2 * dot(N, vR) * N - vR);
    float3 color2 = PhongLighting2(vR,N,R,V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        texColor);
    return float4(saturate(color1 + color2), 1);
}