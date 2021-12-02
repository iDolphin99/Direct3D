// register slot�� buffer 0��° 1��° �� buffer�� �ΰڴ�. 
cbuffer TransformCBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProjection;
}
// Constant buffer�� ����Ǵ� ������ �� shader���� mapping�� �ȴ� 
cbuffer LightBuffer : register(b1)
{
    float3 posLightCS;
    int lightFlag;
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
    output.PosCS = output.Pos.xyz / output.Pos.w; // CS�� position�� ���� �� 

    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    output.Nor = normalize(mul(input.Nor, mul(mView, mWorld)));
    // normal vector�� �� �������� Ȯ���ϱ� ���ؼ� PS_INPUT���� ����, �� normal vector�� ���� cube�� ��ĥ�غ����� 
    // �׷��� �� normal vector�� OS�� noraml��, ���� lighting�� �ϱ⿡ ȿ������ space�� CS�� 
    // �׷��⿡ �� normal vector�� CS�� �����ؾ� �Ѵ�. 
    // �̿� �ش��ϴ� �ڵ�� ���� �������� ��������
    // �ϴ� ���⼭�� scale�� x,y,z ���⿡ ���ؼ� �����ϰ� ���� ������ inverse�� transpose�� ������ �ʰ� 
    // �׳� transform�� ���ϵ��� �ϰ��� 
    // �� �κ��� ������ ���� ���� �����ð��� normal vector�� ��� transform �ϴ����� ���� code�� �����ϼ��� -> extra credit 

    // ���⼭�� view, world�� �ۼ��Ͽ��µ� �̴� column major�̱� ������ �̷� ������ ����� ���̴�. 
    // �׷��� column major�̸� input.Nor�� �����ʿ� ��ġ�ؾ� ���� �ʳ���?
    // �̴� shader������ column major�� �����ϰ�, DirectX Math������ row major�������ϰ� �ֱ� ������ 
    // row major�� ����Ǿ� �ִ� data�� ������ matrix�� ���ϴ� �������� vector�� �պκп� �;� �Ѵ�. -> �� ������ �ȵ�
    // ��, matrix transform �Ǵ� ������ column major�̱� ������ �����ʿ��� �������� �����־�� �Ѵ�! 

    // transform�� �ϰ� �Ǹ� mWorld�� sclae vector�� ���� ������ 
    // Nor vector�� normalized��, unit vector�� �ƴϰ� �� �� �ֱ� ������ normalize�� ��������� �����ش�. 

    // shader�� instruction�� ������ �ִ��� �˻��ϰ� ������ -> hlsl instructino �˻��ؼ� Ȯ���Ͻñ� �ٶ��ϴ�. 
    return output;
}

// color�� output���� ���� phonglighting function 
float3 PhongLighting(float3 L, float3 N, float3 R, float3 V, 
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess, 
    float3 lightColor){
    return float3(0, 1, 1);
}
float3 PhongLighting2(float3 L, float3 N, float3 R, float3 V,
    float3 mtcAmbient, float3 mtcDiffuse, float3 mtcSpec, float shininess,
    float3 lightColor) {
    return float3(0, 1, 1);
}


//--------------------------------------------------------------------------------------
// Pixel Shader 
//--------------------------------------------------------------------------------------
float4 PS(PS_INPUT input) : SV_Target
{
    // ���������� phonglighting�� �ǵ��� ���� �Լ� PhongLighting �Լ��� �Ʒ��� L,N,R,V�� ����ϵ��� �����ϼ��� 

    // Phong lighting�� ���ؼ� �߰������� �ʿ��� ���� �ۼ�
    // Light Color -> ��� ���ǻ� �ϳ��� 
    float3 lightColor = float3(1.f, 1.f, 1.f);// white 
    // Material Color -> ambient, diffuse, specular, shininess 
    float3 mtcAmbient = float3(0.1f, 0.1f, 0.1f); // white, ���ϰ� 
    float3 mtcDiffuse = float3(0.7f, 0.7f, 0);   // yellow, ���ϰ� 
    float3 mtcSpec = float3(0.2f, 0, 0.2f);      // ����� �迭�� 
    float shine = 100.f;
    // Light type (point light)... 
    // light, normal, reflection, veiw vector�� ���� �츮�� ����ؾ� ��...!!! 
    // L�� light position�� ���� �ְ� N�� unit_nor�� �ְ� V�� �� ���� �˼��ְ���? R �����ϼ���! 
    float3 L, N, R, V; 

    float3 colorOut =  PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    // Normal vector�� �׷��� Ȯ���� �� ���� 
    // Normal vector�� �׷��� -1 ~ 1������ ���� ���� color�� 0~1�� ���� ������ ������ ������ ��ȯ�� �־�� �� 
    float3 posCS = input.PosCS;
    float3 unit_nor = normalize(input.Nor);
    return float4(colorOut, 1);
    //return lightFlag == 777 ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1); // flag�� 777�̸� ���� �ƴϸ� ��� ���
}
// Normal vector�� PS�� ���ö��� PS_INPUT�� attribute���� vertex ������ ����Ǿ� �ְ� 
// PS(FS)������ interpolation, scan line conversion algorithm�� ���� interpolation �� ���� ������ �� 
// �׷��� normal vector ���� unit vector ������ ������ ���� �� �ֱ� ������ �̿� ���� ó�����־�� �� -> �׷��� ��������� normalize �Լ��� �ѿ� ����  

// Normal Vector �����ϱ� 
// => OS�� ���ǵ� Normal color�� ������ �ͼ� CS�� normal�� transform�ϰ� transform�� normal�� PS���� ������ ������ �ͱ��� �Ϸ�! 

// Phong lighting �����ϱ� 
// FS���� lighting�� �����Ѵ� -> shading model������ Phong shading�� �����ؾ� �� 
// �׷� Phong shading ������ normal vector �Ӹ� �ƴ϶� �ش� fragment���� 3D geometry�� �ش��ϴ� fragment�� position�� �ʿ��ϴ� 
// �׸��� lighting�� ����ϴ� ������ CS�̹Ƿ� input���� CS�� input�� �ϳ� �� �߰��ȴ�. 

// �� �ʿ��� ���� light position�� �ʿ��� 
// point light�� �� ���ε� WS ���� ������ ��ġ���� point light�� �� �������� 
// �׷� ��� shader���� hard coding �� �� ������ �ش� �ǽ��� camera�� ��ġ�� �ٲ�� ������ application level���� lighting position ���� ���� ��
// �̸� ���ؼ� �߰����� Constant Buffer�� �߰��ϰ���! 
