// register slot에 buffer 0번째 1번째 에 buffer를 두겠다. 
cbuffer TransformCBuffer : register(b0)
{
    matrix mWorld;
    matrix mView;
    matrix mProjection;
}
// Constant buffer에 저장되는 값들이 이 shader에서 mapping이 된다 
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
    output.PosCS = output.Pos.xyz / output.Pos.w; // CS의 position이 들어가게 됨 

    output.Pos = mul(output.Pos, mProjection);
    output.Color = input.Color;
    output.Nor = normalize(mul(input.Nor, mul(mView, mWorld)));
    // normal vector가 잘 나오는지 확인하기 위해서 PS_INPUT으로 뺐고, 이 normal vector를 통해 cube를 색칠해보겠음 
    // 그런데 이 normal vector는 OS의 noraml임, 실제 lighting을 하기에 효율적인 space는 CS임 
    // 그렇기에 이 normal vector를 CS로 변경해야 한다. 
    // 이에 해당하는 코드는 지난 과제에서 제공했음
    // 일단 여기서는 scale이 x,y,z 방향에 대해서 동일하게 들어가기 때문에 inverse의 transpose를 취하지 않고 
    // 그냥 transform을 취하도록 하겠음 
    // 이 부분을 구현할 때는 지난 수업시간에 normal vector를 어떻게 transform 하는지에 대한 code를 적용하세요 -> extra credit 

    // 여기서는 view, world로 작성하였는데 이는 column major이기 때문에 이런 순서로 계산한 것이다. 
    // 그런데 column major이면 input.Nor가 오른쪽에 위치해야 하지 않나요?
    // 이는 shader에서는 column major를 지원하고, DirectX Math에서는 row major를지원하고 있기 때문에 
    // row major로 저장되어 있는 data가 들어오면 matrix를 곱하는 데에서는 vector가 앞부분에 와야 한다. -> 꼭 잊으면 안됨
    // 단, matrix transform 되는 순서는 column major이기 때문에 오른쪽에서 왼쪽으로 곱해주어야 한다! 

    // transform을 하게 되면 mWorld에 sclae vector가 들어가기 때문에 
    // Nor vector가 normalized된, unit vector가 아니게 될 수 있기 때문에 normalize를 명시적으로 취해준다. 

    // shader의 instruction이 무엇이 있는지 검색하고 싶으면 -> hlsl instructino 검색해서 확인하시길 바랍니다. 
    return output;
}

// color를 output으로 내는 phonglighting function 
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
    // 최종적으로 phonglighting이 되도록 위의 함수 PhongLighting 함수와 아래에 L,N,R,V를 계산하도록 구현하세요 

    // Phong lighting을 위해서 추가적으로 필요한 식을 작성
    // Light Color -> 얘는 편의상 하나만 
    float3 lightColor = float3(1.f, 1.f, 1.f);// white 
    // Material Color -> ambient, diffuse, specular, shininess 
    float3 mtcAmbient = float3(0.1f, 0.1f, 0.1f); // white, 약하게 
    float3 mtcDiffuse = float3(0.7f, 0.7f, 0);   // yellow, 강하게 
    float3 mtcSpec = float3(0.2f, 0, 0.2f);      // 보라색 계열로 
    float shine = 100.f;
    // Light type (point light)... 
    // light, normal, reflection, veiw vector를 각각 우리가 계산해야 함...!!! 
    // L은 light position이 위에 있고 N은 unit_nor에 있고 V는 뭐 어케 알수있겠죠? R 구현하세요! 
    float3 L, N, R, V; 

    float3 colorOut =  PhongLighting2(L, N, R, V,
        mtcAmbient, mtcDiffuse, mtcSpec, shine,
        lightColor);

    // Normal vector를 그려서 확인할 수 있음 
    // Normal vector는 그런데 -1 ~ 1사이의 값을 갖고 color는 0~1의 값을 가지기 때문에 강제로 변환해 주어야 함 
    float3 posCS = input.PosCS;
    float3 unit_nor = normalize(input.Nor);
    return float4(colorOut, 1);
    //return lightFlag == 777 ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1); // flag가 777이면 빨강 아니면 녹색 출력
}
// Normal vector가 PS로 들어올때는 PS_INPUT의 attribute들이 vertex 단위로 저장되어 있고 
// PS(FS)에서는 interpolation, scan line conversion algorithm을 통한 interpolation 된 값이 들어오게 됨 
// 그러면 normal vector 값이 unit vector 값으로 들어오지 않을 수 있기 때문에 이에 대해 처리해주어야 함 -> 그래서 명시적으로 normalize 함수를 겉에 씌움  

// Normal Vector 구현하기 
// => OS에 정의된 Normal color를 가지고 와서 CS의 normal로 transform하고 transform된 normal을 PS에서 색으로 입히는 것까지 완료! 

// Phong lighting 구현하기 
// FS에서 lighting을 구현한다 -> shading model에서는 Phong shading을 구현해야 함 
// 그럼 Phong shading 에서는 normal vector 뿐만 아니라 해당 fragment에서 3D geometry에 해당하는 fragment의 position이 필요하다 
// 그리고 lighting을 계산하는 공간이 CS이므로 input으로 CS의 input이 하나 더 추가된다. 

// 더 필요한 것은 light position이 필요함 
// point light을 쓸 것인데 WS 에서 고정된 위치에서 point light를 둘 것이지만 
// 그럴 경우 shader에서 hard coding 할 수 있지만 해당 실습은 camera의 위치가 바뀌기 때문에 application level에서 lighting position 값이 들어가야 함
// 이를 위해서 추가적인 Constant Buffer를 추가하겠음! 
