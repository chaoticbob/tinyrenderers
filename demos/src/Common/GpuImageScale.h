#ifndef TR_GPU_IMAGE_SCALE_H
#define TR_GPU_IMAGE_SCALE_H

#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
#endif

namespace tr {

class GpuImageScale {
public:
  GpuImageScale();
  GpuImageScale(tr_renderer* p_renderer);
  virtual ~GpuImageScale();

  void Scale(tr_cmd* p_cmd);

private:
  tr_descriptor_set*  m_descriptor_set = nullptr;

  tr_shader_program*  m_nearest_shader = nullptr;
  tr_pipeline*        m_nearest_pipeline = nullptr;

  tr_shader_program*  m_linear_shader = nullptr;
  tr_pipeline*        m_linear_pipeline = nullptr;
};

} // namespace tr

#endif // TR_GPU_IMAGE_SCALE_H