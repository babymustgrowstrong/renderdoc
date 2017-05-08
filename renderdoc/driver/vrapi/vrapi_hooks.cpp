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
#include "vrapi_driver.h"

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

void *libvrapi_symHandle = RTLD_NEXT;
WrappedVRAPI *m_VRAPIDriver = nullptr;

//-----------------------------------------------------------------------------------------------------------------
class VRAPIHook : LibraryHook
{
public:
  VRAPIHook()
      : vrapi_EnterVrMode_real(nullptr),
        vrapi_LeaveVrMode_real(nullptr),
        vrapi_SubmitFrame_real(nullptr),
        vrapi_GetTextureSwapChainLength_real(nullptr),
        vrapi_GetTextureSwapChainHandle_real(nullptr),
        vrapi_GetSystemPropertyInt_real(nullptr),
        m_PopulatedHooks(false),
        m_HasHooks(false)
  {
    LibraryHooks::GetInstance().RegisterHook("libvrapi.so", this);

    m_EnabledHooks = true;

    m_VRAPIDriver = nullptr;
  }
  ~VRAPIHook()
  {
    delete m_GLDriver;
    m_VRAPIDriver = nullptr;
  }

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

  WrappedVRAPI *GetDriver()
  {
    if(m_VRAPIDriver == nullptr)
    {
      m_VRAPIDriver = new WrappedVRAPI();
      m_VRAPIDriver->SetWrappedOpenGL(m_GLDriver);
    }

    return m_VRAPIDriver;
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

    return vrapi_SubmitFrame_real != nullptr;
  }

} vrapi_hooks;

//-----------------------------------------------------------------------------------------------------------------
extern "C" {

__attribute__((visibility("default"))) ovrMobile *vrapi_EnterVrMode(const ovrModeParms *parms)
{
  if(vrapi_hooks.vrapi_EnterVrMode_real == nullptr)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrMobile *ovr = vrapi_hooks.vrapi_EnterVrMode_real(parms);

  if(ovr)
    RenderDoc::Inst().AddDeviceFrameCapturer(ovr, vrapi_hooks.GetDriver());

  return ovr;
}

__attribute__((visibility("default"))) void vrapi_LeaveVrMode(ovrMobile *ovr)
{
  if(vrapi_hooks.vrapi_LeaveVrMode_real == nullptr)
  {
    vrapi_hooks.SetupHooks();
  }

  if(ovr)
    RenderDoc::Inst().RemoveDeviceFrameCapturer(ovr);

  vrapi_hooks.vrapi_LeaveVrMode_real(ovr);
}

__attribute__((visibility("default"))) ovrTextureSwapChain *vrapi_CreateTextureSwapChain2(
    ovrTextureType type, ovrTextureFormat format, int width, int height, int levels, int bufferCount)
{
  if(vrapi_hooks.vrapi_CreateTextureSwapChain2_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainHandle_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainLength_real == nullptr)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrTextureSwapChain *texture_swapchain = vrapi_hooks.vrapi_CreateTextureSwapChain2_real(
      type, format, width, height, levels, bufferCount);

  int tex_count = vrapi_hooks.vrapi_GetTextureSwapChainLength_real(texture_swapchain);

  for(int i = 0; i < tex_count; ++i)
  {
    GLuint tex = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(texture_swapchain, i);

    vrapi_hooks.GetDriver()->Common_CreateTextureSwapChain(tex, format, width, height, levels);
  }

  return texture_swapchain;
}

__attribute__((visibility("default"))) ovrTextureSwapChain *vrapi_CreateTextureSwapChain(
    ovrTextureType type, ovrTextureFormat format, int width, int height, int levels, bool buffered)
{
  if(vrapi_hooks.vrapi_CreateTextureSwapChain_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainHandle_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainLength_real == nullptr)
  {
    vrapi_hooks.SetupHooks();
  }

  ovrTextureSwapChain *texture_swapchain =
      vrapi_hooks.vrapi_CreateTextureSwapChain_real(type, format, width, height, levels, buffered);

  int tex_count = vrapi_hooks.vrapi_GetTextureSwapChainLength_real(texture_swapchain);

  for(int i = 0; i < tex_count; ++i)
  {
    GLuint tex = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(texture_swapchain, i);

    vrapi_hooks.GetDriver()->Common_CreateTextureSwapChain(tex, format, width, height, levels);
  }

  return texture_swapchain;
}

__attribute__((visibility("default"))) void vrapi_SubmitFrame(ovrMobile *ovr,
                                                              const ovrFrameParms *parms)
{
  if(vrapi_hooks.vrapi_SubmitFrame_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainLength_real == nullptr ||
     vrapi_hooks.vrapi_GetTextureSwapChainHandle_real == nullptr ||
     vrapi_hooks.vrapi_GetSystemPropertyInt_real == nullptr)
  {
    vrapi_hooks.SetupHooks();
  }

  if(m_GLDriver)
  {
    uint32_t width = 2048;
    uint32_t height = 1024;

    if(parms)
    {
      width = vrapi_hooks.vrapi_GetSystemPropertyInt_real(
                  &parms->Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH) *
              2;
      height = vrapi_hooks.vrapi_GetSystemPropertyInt_real(
          &parms->Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

      // this is just an inline function call, function definition is in VrApi_Ext.h
      const ovrFrameParms *frame_params =
          vrapi_GetFrameParmsConst((const ovrFrameParmsExtBase *)parms);

      if(frame_params->LayerCount > 0)
      {
        // assuming layer 0 is the main layer
        ovrTextureSwapChain *left_swapchain =
            frame_params->Layers[0].Textures[VRAPI_FRAME_LAYER_EYE_LEFT].ColorTextureSwapChain;
        ovrTextureSwapChain *right_swapchain =
            frame_params->Layers[0].Textures[VRAPI_FRAME_LAYER_EYE_RIGHT].ColorTextureSwapChain;

        if(left_swapchain != nullptr || right_swapchain != nullptr)
        {
          SCOPED_LOCK(glLock);

          if(left_swapchain == right_swapchain)
          {
            GLuint gl_handle = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(left_swapchain, 0);

            uint32_t tex_width, tex_height;
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                gl_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_WIDTH, (GLint *)&tex_width);
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                gl_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_HEIGHT, (GLint *)&tex_height);

            width = tex_width;
            height = tex_height;
          }
          else
          {
            GLuint left_handle = vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(left_swapchain, 0);
            GLuint right_handle =
                vrapi_hooks.vrapi_GetTextureSwapChainHandle_real(right_swapchain, 0);

            uint32_t left_width, left_height, right_width, right_height;
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                left_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_WIDTH, (GLint *)&left_width);
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                left_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_HEIGHT, (GLint *)&left_height);
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                right_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_WIDTH, (GLint *)&right_width);
            m_GLDriver->GetHookset().glGetTextureLevelParameterivEXT(
                right_handle, eGL_TEXTURE_2D, 0, eGL_TEXTURE_HEIGHT, (GLint *)&right_height);

            RDCASSERT(left_width == right_width);
            RDCASSERT(left_height == right_height);

            width = left_width + right_width;
            height = left_height;
          }
        }
      }
    }

    m_GLDriver->WindowSize(nullptr, width, height);
  }

  vrapi_hooks.GetDriver()->SubmitFrame(ovr);

  vrapi_hooks.vrapi_SubmitFrame_real(ovr, parms);
}
}