/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2017 Baldur Karlsson
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
******************************************************************************/

#include <dlfcn.h>
#include "common/threading.h"
#include "driver/gl/gl_common.h"
#include "driver/gl/gl_driver.h"
#include "driver/gl/gl_hooks_linux_shared.h"
#include "hooks/hooks.h"
#include "official/VrApi_Ext.h"
#include "official/VrApi_Types.h"

//-----------------------------------------------------------------------------------------------------------------
typedef ovrMobile *(*PFN_vrapi_EnterVrMode)(const ovrModeParms *);
typedef void (*PFN_vrapi_LeaveVrMode)(ovrMobile *);
typedef void (*PFN_vrapi_SubmitFrame)(ovrMobile *, const ovrFrameParms *);
typedef int (*PFN_vrapi_GetTextureSwapChainLength)(ovrTextureSwapChain *);
typedef unsigned int (*PFN_vrapi_GetTextureSwapChainHandle)(ovrTextureSwapChain *, int);
typedef int (*PFN_vrapi_GetSystemPropertyInt)(const ovrJava *, const ovrSystemProperty);
typedef ovrTextureSwapChain *(*PFN_vrapi_CreateTextureSwapChain2)(ovrTextureType, ovrTextureFormat,
                                                                  int, int, int, int);
typedef ovrTextureSwapChain *(*PFN_vrapi_CreateTextureSwapChain)(ovrTextureType, ovrTextureFormat,
                                                                 int, int, int, bool);

//-----------------------------------------------------------------------------------------------------------------
void *libvrapi_symHandle = RTLD_NEXT;

//-----------------------------------------------------------------------------------------------------------------
class VRAPIHook : LibraryHook
{
public:
  VRAPIHook()
      : vrapi_EnterVrMode_real(NULL),
        vrapi_LeaveVrMode_real(NULL),
        vrapi_SubmitFrame_real(NULL),
        vrapi_GetTextureSwapChainLength_real(NULL),
        vrapi_GetTextureSwapChainHandle_real(NULL),
        vrapi_GetSystemPropertyInt_real(NULL),
        m_PopulatedHooks(false),
        m_HasHooks(false)
  {
    LibraryHooks::GetInstance().RegisterHook("libvrapi.so", this);

    m_EnabledHooks = true;
  }
  ~VRAPIHook() {}
  bool CreateHooks(const char *libName)
  {
    if(!m_EnabledHooks)
      return false;

    if(libName)
      PosixHookLibrary(libName, &libHooked);

    bool success = SetupHooks();

    if(!success)
      return false;

    m_HasHooks = true;

    return true;
  }

  void EnableHooks(const char *libName, bool enable) { m_EnabledHooks = enable; }
  void OptionsUpdated(const char *libName) {}
  void SetupExportedFunctions()
  {
    if(RenderDoc::Inst().IsReplayApp())
      SetupHooks();
  }

  static void libHooked(void *realLib) { libvrapi_symHandle = realLib; }
  //---------------------------------------------------------------------------------------------------------
  PFN_vrapi_EnterVrMode vrapi_EnterVrMode_real;
  PFN_vrapi_LeaveVrMode vrapi_LeaveVrMode_real;
  PFN_vrapi_CreateTextureSwapChain2 vrapi_CreateTextureSwapChain2_real;
  PFN_vrapi_CreateTextureSwapChain vrapi_CreateTextureSwapChain_real;
  PFN_vrapi_SubmitFrame vrapi_SubmitFrame_real;

  PFN_vrapi_GetTextureSwapChainLength vrapi_GetTextureSwapChainLength_real;
  PFN_vrapi_GetTextureSwapChainHandle vrapi_GetTextureSwapChainHandle_real;
  PFN_vrapi_GetSystemPropertyInt vrapi_GetSystemPropertyInt_real;

  bool m_PopulatedHooks;
  bool m_HasHooks;
  bool m_EnabledHooks;

  bool SetupHooks()
  {
    if(vrapi_EnterVrMode_real == NULL)
      vrapi_EnterVrMode_real =
          (PFN_vrapi_EnterVrMode)dlsym(libvrapi_symHandle, "vrapi_EnterVrMode");
    if(vrapi_LeaveVrMode_real == NULL)
      vrapi_LeaveVrMode_real =
          (PFN_vrapi_LeaveVrMode)dlsym(libvrapi_symHandle, "vrapi_LeaveVrMode");
    if(vrapi_CreateTextureSwapChain2_real == NULL)
      vrapi_CreateTextureSwapChain2_real = (PFN_vrapi_CreateTextureSwapChain2)dlsym(
          libvrapi_symHandle, "vrapi_CreateTextureSwapChain2");
    if(vrapi_CreateTextureSwapChain_real == NULL)
      vrapi_CreateTextureSwapChain_real = (PFN_vrapi_CreateTextureSwapChain)dlsym(
          libvrapi_symHandle, "vrapi_CreateTextureSwapChain");
    if(vrapi_SubmitFrame_real == NULL)
      vrapi_SubmitFrame_real =
          (PFN_vrapi_SubmitFrame)dlsym(libvrapi_symHandle, "vrapi_SubmitFrame");

    if(vrapi_GetTextureSwapChainLength_real == NULL)
      vrapi_GetTextureSwapChainLength_real = (PFN_vrapi_GetTextureSwapChainLength)dlsym(
          libvrapi_symHandle, "vrapi_GetTextureSwapChainLength");
    if(vrapi_GetTextureSwapChainHandle_real == NULL)
      vrapi_GetTextureSwapChainHandle_real = (PFN_vrapi_GetTextureSwapChainHandle)dlsym(
          libvrapi_symHandle, "vrapi_GetTextureSwapChainHandle");
    if(vrapi_GetSystemPropertyInt_real == NULL)
      vrapi_GetSystemPropertyInt_real =
          (PFN_vrapi_GetSystemPropertyInt)dlsym(libvrapi_symHandle, "vrapi_GetSystemPropertyInt");

    return vrapi_SubmitFrame_real != NULL;
  }

} vrapi_hooks;

//-----------------------------------------------------------------------------------------------------------------
GLenum GetInternalFormat(ovrTextureFormat ovr_format)
{
  GLenum internalFormat = eGL_RGBA8;

  GLenum conversion_table[] = {
      eGL_RGBA8,                // VRAPI_TEXTURE_FORMAT_NONE,
      eGL_RGB565,               // VRAPI_TEXTURE_FORMAT_565,
      eGL_RGB5_A1,              // VRAPI_TEXTURE_FORMAT_5551,
      eGL_RGBA4,                // VRAPI_TEXTURE_FORMAT_4444,
      eGL_RGBA8,                // VRAPI_TEXTURE_FORMAT_8888,
      eGL_SRGB8_ALPHA8,         // VRAPI_TEXTURE_FORMAT_8888_sRGB,
      eGL_RGBA16F,              // VRAPI_TEXTURE_FORMAT_RGBA16F,
      eGL_DEPTH_COMPONENT16,    // VRAPI_TEXTURE_FORMAT_DEPTH_16,
      eGL_DEPTH_COMPONENT24,    // VRAPI_TEXTURE_FORMAT_DEPTH_24,
      eGL_DEPTH24_STENCIL8      // VRAPI_TEXTURE_FORMAT_DEPTH_24_STENCIL_8,
  };

  RDCASSERT(ovr_format < sizeof(conversion_table) / sizeof(conversion_table[0]));

  if(ovr_format < sizeof(conversion_table) / sizeof(conversion_table[0]))
  {
    internalFormat = conversion_table[ovr_format];
  }

  return internalFormat;
}

TextureCategory GetTextureCategory(ovrTextureFormat ovr_format)
{
  TextureCategory category = TextureCategory::ColorTarget | TextureCategory::SwapBuffer;

  TextureCategory conversion_table[] = {
      TextureCategory::NoFlags,                                      // VRAPI_TEXTURE_FORMAT_NONE,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_565,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_5551,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_4444,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_8888,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_8888_sRGB,
      TextureCategory::ColorTarget | TextureCategory::SwapBuffer,    // VRAPI_TEXTURE_FORMAT_RGBA16F,
      TextureCategory::DepthTarget,    // VRAPI_TEXTURE_FORMAT_DEPTH_16,
      TextureCategory::DepthTarget,    // VRAPI_TEXTURE_FORMAT_DEPTH_24,
      TextureCategory::DepthTarget     // VRAPI_TEXTURE_FORMAT_DEPTH_24_STENCIL_8,
  };

  RDCASSERT(ovr_format < sizeof(conversion_table) / sizeof(conversion_table[0]));

  if(ovr_format < sizeof(conversion_table) / sizeof(conversion_table[0]))
  {
    category = conversion_table[ovr_format];
  }

  return category;
}

//-----------------------------------------------------------------------------------------------------------------
extern "C" {

__attribute__((visibility("default"))) ovrMobile *vrapi_EnterVrMode(const ovrModeParms *parms)
{
  if(vrapi_hooks.vrapi_EnterVrMode_real == NULL)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrMobile *ovr = vrapi_hooks.vrapi_EnterVrMode_real(parms);

  if(m_GLDriver)
  {
    void *ctx = m_GLDriver->GetCtx();
    void *wndHandle = ovr;

    RenderDoc::Inst().AddFrameCapturer(ctx, wndHandle, m_GLDriver);
    RenderDoc::Inst().SetActiveWindow(ctx, wndHandle);
  }

  return ovr;
}

__attribute__((visibility("default"))) void vrapi_LeaveVrMode(ovrMobile *ovr)
{
  if(vrapi_hooks.vrapi_LeaveVrMode_real == NULL)
  {
    vrapi_hooks.SetupHooks();
  }

  vrapi_hooks.vrapi_LeaveVrMode_real(ovr);
}

__attribute__((visibility("default"))) ovrTextureSwapChain *vrapi_CreateTextureSwapChain2(
    ovrTextureType type, ovrTextureFormat format, int width, int height, int levels, int bufferCount)
{
  if(vrapi_hooks.vrapi_CreateTextureSwapChain2_real == NULL ||
     vrapi_hooks.vrapi_GetTextureSwapChainHandle_real == NULL ||
     vrapi_hooks.vrapi_GetTextureSwapChainLength_real == NULL)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrTextureSwapChain *texture_swapchain = vrapi_hooks.vrapi_CreateTextureSwapChain2_real(
      type, format, width, height, levels, bufferCount);

  int tex_count = vrapi_hooks.vrapi_GetTextureSwapChainLength_real(texture_swapchain);

  if(m_GLDriver)
  {
    for(int i = 0; i < tex_count; ++i)
    {
      GLuint tex = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(texture_swapchain, i);
      GLenum internalformat = GetInternalFormat(format);
      TextureCategory creationFlags = GetTextureCategory(format);

      m_GLDriver->CreateVRAPITextureSwapChain(tex, internalformat, width, height, creationFlags);
    }
  }

  return texture_swapchain;
}

__attribute__((visibility("default"))) ovrTextureSwapChain *vrapi_CreateTextureSwapChain(
    ovrTextureType type, ovrTextureFormat format, int width, int height, int levels, bool buffered)
{
  if(vrapi_hooks.vrapi_CreateTextureSwapChain_real == NULL ||
     vrapi_hooks.vrapi_GetTextureSwapChainHandle_real == NULL ||
     vrapi_hooks.vrapi_GetTextureSwapChainLength_real == NULL)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrTextureSwapChain *texture_swapchain =
      vrapi_hooks.vrapi_CreateTextureSwapChain_real(type, format, width, height, levels, buffered);

  int tex_count = vrapi_hooks.vrapi_GetTextureSwapChainLength_real(texture_swapchain);

  for(int i = 0; i < tex_count; ++i)
  {
    GLuint tex = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(texture_swapchain, i);
    GLenum internalformat = GetInternalFormat(format);
    TextureCategory creationFlags = GetTextureCategory(format);

    m_GLDriver->CreateVRAPITextureSwapChain(tex, internalformat, width, height, creationFlags);
  }

  return texture_swapchain;
}

__attribute__((visibility("default"))) void vrapi_SubmitFrame(ovrMobile *ovr,
                                                              const ovrFrameParms *parms)
{
  if(vrapi_hooks.vrapi_SubmitFrame_real == NULL || vrapi_hooks.vrapi_GetSystemPropertyInt_real == NULL)
  {
    vrapi_hooks.SetupHooks();
  }

  if(m_GLDriver)
  {
    SCOPED_LOCK(glLock);

    int width = vrapi_hooks.vrapi_GetSystemPropertyInt_real(&parms->Java,
                                                            VRAPI_SYS_PROP_DISPLAY_PIXELS_WIDE);
    int height = vrapi_hooks.vrapi_GetSystemPropertyInt_real(&parms->Java,
                                                             VRAPI_SYS_PROP_DISPLAY_PIXELS_HIGH);

    m_GLDriver->WindowSize(ovr, width, height);
    m_GLDriver->SwapBuffers(ovr);
  }

  vrapi_hooks.vrapi_SubmitFrame_real(ovr, parms);
}
}