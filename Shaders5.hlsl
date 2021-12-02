 cbuffer TransformCBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProjection;
}
// HW2.4) Register hard coding variables to Constant Buffers
cbuffer LightCBuffer : register(b1)
{
    float3 posLightCS; //12
    int lightFlag;     //4
    
    float3 lightColor; //12
    int dummy1;        //4
}
// Material property 
cbuffer MtCBuffer : register(b2)
{
    float3 mtcAmbient; //12
    float shine;       //4

    float3 mtcDiffuse; //12
    float dummy2;      //4
    float3 mtcSpec;    //12
    float dummy3;      //4
}


struct VS_INPUT
{
    float4 Pos : POSITION0;
    float4 Color : COLOR0;
    float3 Nor : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color: COLOR0;
    float3 Nor : NORMAL;
    float3 PosCS : POSITION0;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS_TEST(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.PosCS = output.Pos.xyz / output.Pos.w; 

    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    output.Nor = normalize(mul(input.Nor, mul(mView, mWorld)));
 
    return output;
}


// HW2) Lighting your cube with your own light object -> PhongLighting  
float3 PhongLighting(float3 L, float3 N, float3 R, float3 V, 
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess, 
    float3 lightColor){

    return mtcAmbient * lightColor + mtcDiffuse * lightColor * max(dot(N, L), 0) + mtcSpec * lightColor * pow(max(dot(R, V), 0), shininess);
}
// HW2.3) Resolve the undesirable specular lighting on the opposite face of the cube to the light
float3 PhongLighting2(float3 L, float3 N, float3 R, float3 V,
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess,
    float3 lightColor) {

    if (dot(N, L) <= 0) R = (float3)0;

    return mtcAmbient * lightColor + mtcDiffuse * lightColor * max(dot(N, L), 0) + mtcSpec * lightColor * pow(max(dot(R, V),0), shininess);
}


//--------------------------------------------------------------------------------------
// Pixel Shader 
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    float3 unit_nor = normalize(input.Nor);

    float3 L, N, R, V;
    L = normalize(posLightCS - input.PosCS);
    N = unit_nor;
    R = normalize(2 * (dot(N, L)) * N - L);
    V = normalize(-input.PosCS);

    // HW2.5) Add an option to the scene property : Paralle lighting or Point lighting
    if (lightFlag == 1) L = normalize(posLightCS - input.PosCS);
    if (lightFlag == 2) L = normalize(float3(0,1,0));

    float3 colorOut = PhongLighting(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    return float4(colorOut, 1);
}