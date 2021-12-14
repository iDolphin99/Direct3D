cbuffer TransformScene : register(b0)
{
    matrix g_mView;
    matrix g_mProjection;
}

cbuffer TransformModel : register(b1)
{
    // VS���� ����ϴ� �� 
    matrix g_mWorld; 

    // Phongshading�� ���� PS���� ����ϴ� �� 
    int g_mtAmbient, g_mtDiffuse, g_mtSpec; // 4 4 4 
    float g_shininess; // 4 => 16 // 
}


cbuffer LightBuffer : register(b2)
{
    float3 g_posLightCS;
    int g_lightColor;
    float3 g_dirLightCS;
    int g_lightFlag;

    matrix g_mView2EnvOS;
}

// register -> shader resource�� t(texture)�� ��, conventionally
// application level���� shader�� Buffer t0���ٰ� Normal buffer resource�� ����Ű�� view�� set �ؾ� �Ѵ� 
// �׷��� �����ν� shader���� NoramlBuffer�� indexing �� �� �ְ� �ȴ�
Buffer<float3> NormalBuffer : register(t0);

// float3? unsigned char? 4channel�ε� �� float? 
// R8G8B8A8_UNORM -> UNORM �ϸ� 0~1�� Normalize�� float�� ���ڴٴ� �ǹ�, GPU���� ���� ���е��� 8bit�� float���� ���ڴٴ� �ǹ� 
// �̷��� �ϸ� 0->0, 255->1.0���� mapping�� float���� ������ ���� ���� 
// ���� ���⼭ float2�� �ϸ� ���������� R,G���� ����� �� ����, A���� ���� �����Ŷ� float3�ص� ��  
Texture2D<float4> EnvTexture : register(t1);

SamplerState EnvSamplerCpp : register(s0);

// Sampler ��ü�� Resource�� ���� set���൵ �� -> 1: 04: 41�� ���� 
// sampler�� Semantics�� s�� �����ؾ� �Ѵ�
SamplerState EnvSampler : register(s16)
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
// position ������ ������ PS_INPUT1�� ���� �ξ ���� cube model�� ���� �ִ� PS�� ���� ���� ���� ������� 
PS_INPUT1 VS_P(float4 Pos : POSITION0)
{
    PS_INPUT1 output = (PS_INPUT1)0;    
    output.Pos = mul(Pos, g_mWorld);
    output.Pos = mul(output.Pos, g_mView);
    output.PosCS = output.Pos.xyz / output.Pos.w;

    output.Pos = mul(output.Pos, g_mProjection);
    output.Color = (float4)0; // ���� issue�� ó���� 
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
// SV_PrimitiveID ��, System value Semantics�� Raterizer stage���� �ִ� system value��, �׷��� VS input ������ �� �� ���� PS input���� �־���� �� 
// �׷��ٰ� PS_INPUT���� �����ϸ� �Ǵ°� �ƴ�, �� strut�� VS_OUTPUT�̱� ���� 
// �׷��� �ش� semantics�� ������ ���� PS�� �ۼ��Ͽ� ����� 
// shading�� �ϰ� �ִ� PS�� shading�� CS���� �ϰ� ���� -> Space���� missmatching 
// CS ������ �������� lighting�� �̷������ �ϱ� ������ ������ �߻��� 
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

        // Vertex�� attribute�� ���� �ִ� noraml ���� Rasterizer���� interpolation�� �� 
        //N = normalize(input.Nor);
        // 0, 1 -> index 0, 1, 2 -> index 1 
        // �׷��� �ش� noraml �� OS�� �� -> Normal vector�� CS�� ������ ��ȯ����� ��
        // �̷��� ���������ν� ��(face)������ normal vector�� ���� �� 
        // ���� �̰� �Ʊ� ������ stl file�� �о ���� �߰��ϴ� �ڵ带 �ۼ�����! 
        // C standard library���� parsing�� �����ν� stl�� import�ϴ� function�� ���� ���� �� ���� 
        // �ϴ��� github�� stl_reader ���� ���� �ڵ���� �ֱ� ������ �װ� �����ٰ� ���ڽ��ϴ�
        N = normalize(mul(NormalBuffer[primID], mul(g_mView, g_mWorld)));
        // �ٵ� ������ Normal�� �̻��ϰ� �а� ����, ������2 
        // ����ü cube�� �׸����� �� ���� �ﰢ���� �ϳ��� normal�� �����ϴ� �� stl model�� �ϳ��� traiangle�� �ϳ��� normal�� ������ ���� -> �׷��Ƿ� ������ 2�� ����� �� 
        // �׷� ť��� ���� Shader�� ������� �� �ִµ� �츮�� application level���� Normal�� ���� �� ���� 
 
        R = 2 * dot(N, L) * N - L;
    }

    float3 colorOut = PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    // Rasterizer stage���� primitiveID�� fragment�� � primitiveID�� ���ϴ� ���� Ȯ���� �� �ְ� �ϴ� test code
    // �� ������ ������ normal vector�� indexing�ϴ� ������ �ش� ID�� ����� �� ���� 
    // �׷� PS���� �ش� noraml buffer�� indexing �ؾ� �� 
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
    float3 colorOut = ConvertInt2Float3(g_lightColor);

    return float4(colorOut, 1);
}

// 
float4 PS3(PS_INPUT2 input) : SV_Target
{
    //float3 colorOut = ConvertInt2Float3(g_lightColor);

    // u, v ���� ���ؼ� mapping�� ���� ������ 
    //float3 colorOut = float3(input.Tex.xy, 0);

    // Sampler�� sampling �ϴ� ��� -> �ش� x,y���� ���ؼ� �о�ͼ� mapping�� 
    // �츮�� ���� ������ z ���̱� ������ map�� �ϴù������ ���̴� ���� ���� 
    float3 colorOut = EnvTexture.Sample(EnvSamplerCpp, input.Tex.xy);

    return float4(colorOut, 1);
}

float4 PS4(PS_INPUT1 input, uint primID : SV_PrimitiveID) : SV_Target
{
    // Cube�� Texture coordinate�� �ȵ����� Reflection vector�κ��� u,v�� ����ؾ� �ϰ� 
    // u,v�� ����ϴ� model�� Blinn/Newell Latitude Mapping method�� ����� ���� 

    // Light Color 
    float3 lightColor = ConvertInt2Float3(g_lightColor);// float3(1.f, 1.f, 1.f);
    // Material Colors : ambient, diffuse, specular, shininess
    float3 mtcAmbient = ConvertInt2Float3(g_mtAmbient), mtcDiffuse = ConvertInt2Float3(g_mtDiffuse), mtcSpec = ConvertInt2Float3(g_mtSpec);
    float shine = g_shininess;
    // Light type (point)... 
    float3 L, N, R, V; // compute to do

    // Reflection vector�� ���ϴ� ��� 
    {
        if (g_lightFlag == 0)
            L = normalize(g_posLightCS - input.PosCS);
        else
            L = g_dirLightCS;
        V = normalize(-input.PosCS);

        //N = normalize(input.Nor);
        // 0, 1 ==> 0
        // 1, 2 ==> 1
        N = normalize(mul(NormalBuffer[primID], mul(g_mView, g_mWorld))); // ������� ���� ��� ����

        R = 2 * dot(N, L) * N - L;
    }

    // Reflection vector�� ���ϴ� ��� 
    // �ش� vector�� CS�� vector�� �� vector�� Hyper sphere�� OS�� �������� matrix�� ���ؼ� Constant Buffer�� ���� ���� -> CBLight 
    // ȯ�� map���� �������� Texture sample�� light���� ���ڴٰ� �����Ƿ�
    // �ƹ�ư viewmatrix 2 envmatrix �� ������, ������ transform��������, x,y,z���� ���Ա� ������ �̷κ��� u,v�� ����ϴ� ���� Ȯ���ϸ�
    // r �� unit�̶� ���� 1�̶� ġ�� -> theta�� phi�� ����ؾ� �� 
    // z�� sin(phi)�κ��� phi�� ���� �� ���� 
    // phi�� ������ �߽����� �浵�� ���� ���� phi�� �� 
    // �츮�� v���� 0->1, ���������� �Ʒ������� ���� ������ �� 
    float3 vR = 2 * dot(N, V) * N - V;
    vR = normalize(vR);

    float3 color1 = PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    // �긦 ������ lighting�Ϸ��� �ۼ��� �� 
    float3 envR = mul(vR, g_mView2EnvOS);
    envR = normalize(envR);

    float X_PI = 3.14f;
    float phi = asin(envR.z); // 90~-90
    float cosphi = cos(phi);
    float theta = acos(max(min(envR.x / cos(phi),0.999f),-0.999f)); // 0 ~ pi
    float u = envR.y >= 0 ? theta / (2 * X_PI) : (2 * X_PI - theta) / (2 * X_PI);
    float v = (X_PI / 2 - phi) / X_PI;

    // u,v�� ����ϰ� u,v�κ��� mapping�ϴ� code
    //float3 texColor = EnvTexture.Sample(EnvSamplerCpp, input.Tex.xy);
    float3 texColor = EnvTexture.Sample(EnvSamplerCpp, float2(u, v));

    // �̶� R vector�� light vector �� �������ϰ�, light vector�� ���ؼ� reflection vector�� ���ؾ� �ϴϱ� �ٽ� ������� �� 
    // ���⼭ lighting�� view space���� �̷����� ������ ������
    // lighting property�� ������ ���ٴ� �����Ͽ� ���� property�� �״�� �ְ� light color�� texture color�� �ִ� ���̴�!! 
    R = normalize(2 * dot(N, vR) * N - vR);
    float3 color2 = PhongLighting2(vR,N,R,V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        texColor);
    return float4(saturate(color1 + color2), 1);
}