#define IMAGE_SIZE_X        1024
#define IMAGE_SIZE_Y        1024
#define KERNEL_SIZE         13
#define HALF_KERNEL_SIZE    6

#define NUM_THREADS_X       1024
#define NUM_THREADS_Y       1
#define NUM_THREADS_Z       1

static const float k_sample_weights[KERNEL_SIZE] = {
  0.002216,
  0.008764,
  0.026995,
  0.064759,
  0.120985,
  0.176033,
  0.199471,
  0.176033,
  0.120985,
  0.064759,
  0.026995,
  0.008764,
  0.002216,
};

Texture2D<float4>   BufferIn  : register(t0);
RWTexture2D<float4> BufferOut : register(u1);

groupshared float4  g_shared_input[NUM_THREADS_X];

//! @fn hblur_main
//!
//!
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void hblur_main(uint3 gid : SV_GroupID, uint gindex : SV_GroupIndex)
{
  int2 coord = int2(gindex, gid.y);
  g_shared_input[gindex] =  BufferIn.Load(int3(coord, 0));

  GroupMemoryBarrierWithGroupSync();

  float4 value = 0;
  for (int i = -HALF_KERNEL_SIZE; i < HALF_KERNEL_SIZE; ++i) {
    int index = gindex + i;
    if (index >= 0 && index < NUM_THREADS_X) {
      value += g_shared_input[index] * k_sample_weights[i + HALF_KERNEL_SIZE];
    }    
  }
  BufferOut[coord] = value;
}

//! @fn vblur_main
//!
//!
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void vblur_main(uint3 gid : SV_GroupID, uint gindex : SV_GroupIndex)
{
  int2 coord = int2(gid.x, gindex);
  g_shared_input[gindex] =  BufferIn.Load(int3(coord, 0));

  GroupMemoryBarrierWithGroupSync();

  float4 value = 0;
  for (int i = -HALF_KERNEL_SIZE; i < HALF_KERNEL_SIZE; ++i) {
    int index = gindex + i;
    if (index >= 0 && index < NUM_THREADS_X) {
      value += g_shared_input[index] * k_sample_weights[i + HALF_KERNEL_SIZE];
    }    
  }
  BufferOut[coord] = value;
}
