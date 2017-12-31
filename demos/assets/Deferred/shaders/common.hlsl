

/*! @struct VSInput

*/
struct VSInput {
  float3  PositionOS  : POSITION;
  float3  NormalOS    : NORMAL;
  float2  TexCoord    : TEXCOORD;
};

/*! @struct VSOutput

*/
struct VSOutput {
  float4  PositionCS  : SV_Position;
  float4  PositionWS  : POSITION;
  float3  NormalWS    : NORMAL;
  float2  TexCoord    : TEXCOORD0;
};

/*! @struct PSInput

*/
typedef VSOutput PSInput;

/*! @struct PSDeferredOutput

*/
struct PSDeferredOutput {
  /*

  */
  float4  oColor0 : SV_TARGET0;
  
  /*


  */
  float4  oColor1 : SV_TARGET1;
  
  /*


  */
  float4  oColor2 : SV_TARGET2;
  
  /*


  */
  float4  oColor3 : SV_TARGET3;
};

/*! @struct ViewData

*/
struct ViewData {
  float4x4  ViewMatrix;
  float4x4  ProjectionMatrix;
  float4x4  ViewProjectionMatrix;
  float3    ViewDirection;
};

/*! @struct TransformData

*/
struct TransformData {
  float4x4  ModelMatrix;
  float4x4  ModelViewMatrix;
  float4x4  ModelViewProjectionMatrix;
  float3x3  NormalMatrixWS;
  float3x3  NormalMatrixVS;
  float3    DebugColor;
};


/*! @struct BRDFMaterialData

*/
struct BRDFMaterialData {
  float3  BaseColor;
  float   Metallic;
  float   Subsurface;
  float   Specular;
  float   Roughness;
  float   SpecularTint;
  float   Anisotropic;
  float   Sheen;
  float   SheenTint;
  float   ClearCoat;
  float   ClearCoatGloss;
  float   kA;
  float   kD;
  float   kS;
};
