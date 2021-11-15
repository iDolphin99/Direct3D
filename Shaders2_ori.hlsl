// constantbuffer�� parameter�� �������� ��� 
// cbuffer ��� Ű����, register(b0) : ù ��° buffer�� set�ϰڴ� 
// shader �������� �̸��� ConstatnBuffer�� �ȴ� 
// ���� register(b1)�� ���� �̸��� �ٸ� ���� ����ؾ� �Ѵ� 
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

// VS_OUPUT ��� ���� 
struct PS_INPUT
{
    // SV_POSITION�� projection space�� ���ǵǾ� ������ raterize r�� ���� �ڵ����� viewport transform�� ���� SS space ������ ���Ͽ� fragment shader input���� ���� �� 
    float4 Pos : SV_POSITION; // semantics : �ش� type variable�� � �뵵�� ���̴� �� : string, �ش� variable�� �뵵�� ���� syntax�� ���� 
    float4 Color: COLOR0;
};



//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS_TEST(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0; // �ش� struct�� variable�� 0���� �ʱ�ȭ�ϰڴ� 
    output.Pos = mul(input.Pos, mWorld);
    output.Pos = mul(output.Pos, mView);
    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    return output; // �ϳ��� input�� ���ؼ� �������� output�� return �� �� ���� 
}
// �ش� shader�� ���� ���� input, output�� ���� �� ���� (fragment shader�� ��������)
// �ﰢ���� ���ؼ��� �ϳ��� input�� �ް� output���� system position�� �״�� ������ ä ������ �� ����  
// float4 Pos : POSITION0 �̶� POSITION �̶� ���� (doc) 



//--------------------------------------------------------------------------------------
// Pixel Shader 
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    return float4(input.Color.xyz, 1.0f);    // Yellow, with Alpha = 1
}
// return float4(input.PosTemp.xy + (float2)-.5, 1.0f, 1.0f); 
// 2d vector + 2d vector �� �ȴ� 
// �׸��� �⺻������ 3d �̿��� 3d-razister�� homogeneous ��(1)�� ���� 4d ���� ���� ������, �׷��� rasterizer�� output�� input���� �޴� PS�� input�� 4d �̿��� ����???