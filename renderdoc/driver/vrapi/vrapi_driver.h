
#pragma once

#include "core/core.h"
#include "driver/gl/official/glcorearb.h"
#include "driver/vrapi/official/VrApi_Types.h"

class WrappedOpenGL;

class WrappedVRAPI : public IFrameCapturer
{
public:
  WrappedVRAPI();
  ~WrappedVRAPI();

  void StartFrameCapture(void *dev, void *wnd);
  bool EndFrameCapture(void *dev, void *wnd);

  void SetWrappedOpenGL(WrappedOpenGL *wrappedOpenGL);
  //-------------------------------------------------------------------------
  void Common_CreateTextureSwapChain(GLuint texture_handle, ovrTextureFormat format, int width,
                                     int height, int levels);
  void SubmitFrame(void *ovrMobile);

private:
  WrappedOpenGL *m_pWrappedOpenGL;
  // TODO: WrappedVulkan * m_pWrappedVulkan;

  bool m_AppControlledCapture;
};