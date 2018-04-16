
cbuffer InMatrices {
  column_major float4x4 col_matrix;
  row_major    float4x4 row_matrix;
};

struct OutMatrixData {
  float   matrix0[16];
  float   matrix1[16];
  float4  vec0;
  float4  vec1;
};

RWStructuredBuffer<OutMatrixData> OutMatrices : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  OutMatrices[0].matrix0[ 0] = col_matrix._m00;
  OutMatrices[0].matrix0[ 1] = col_matrix._m10;
  OutMatrices[0].matrix0[ 2] = col_matrix._m20;
  OutMatrices[0].matrix0[ 3] = col_matrix._m30;
  OutMatrices[0].matrix0[ 4] = col_matrix._m01;
  OutMatrices[0].matrix0[ 5] = col_matrix._m11;
  OutMatrices[0].matrix0[ 6] = col_matrix._m21;
  OutMatrices[0].matrix0[ 7] = col_matrix._m31;
  OutMatrices[0].matrix0[ 8] = col_matrix._m02;
  OutMatrices[0].matrix0[ 9] = col_matrix._m12;
  OutMatrices[0].matrix0[10] = col_matrix._m22;
  OutMatrices[0].matrix0[11] = col_matrix._m32;
  OutMatrices[0].matrix0[12] = col_matrix._m03;
  OutMatrices[0].matrix0[13] = col_matrix._m13;
  OutMatrices[0].matrix0[14] = col_matrix._m23;
  OutMatrices[0].matrix0[15] = col_matrix._m33;

  OutMatrices[0].matrix1[ 0] = row_matrix._m00;
  OutMatrices[0].matrix1[ 1] = row_matrix._m10;
  OutMatrices[0].matrix1[ 2] = row_matrix._m20;
  OutMatrices[0].matrix1[ 3] = row_matrix._m30;
  OutMatrices[0].matrix1[ 4] = row_matrix._m01;
  OutMatrices[0].matrix1[ 5] = row_matrix._m11;
  OutMatrices[0].matrix1[ 6] = row_matrix._m21;
  OutMatrices[0].matrix1[ 7] = row_matrix._m31;
  OutMatrices[0].matrix1[ 8] = row_matrix._m02;
  OutMatrices[0].matrix1[ 9] = row_matrix._m12;
  OutMatrices[0].matrix1[10] = row_matrix._m22;
  OutMatrices[0].matrix1[11] = row_matrix._m32;
  OutMatrices[0].matrix1[12] = row_matrix._m03;
  OutMatrices[0].matrix1[13] = row_matrix._m13;
  OutMatrices[0].matrix1[14] = row_matrix._m23;
  OutMatrices[0].matrix1[15] = row_matrix._m33;


  OutMatrices[0].vec0 = col_matrix[1];
  OutMatrices[0].vec1 = row_matrix[1];
}
