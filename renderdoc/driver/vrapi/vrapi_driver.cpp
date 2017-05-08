#include "vrapi_driver.h"
#include "driver/gl/gl_driver.h"

//------------------------------------------------------------------------------------------------------
WrappedVRAPI::WrappedVRAPI() : m_pWrappedOpenGL(nullptr), m_AppControlledCapture(false)
{
  if(RenderDoc::Inst().GetCrashHandler())
    RenderDoc::Inst().GetCrashHandler()->RegisterMemoryRegion(this, sizeof(WrappedVRAPI));
}
WrappedVRAPI::~WrappedVRAPI()
{
}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::StartFrameCapture(void *dev, void *wnd)
{
  if(m_pWrappedOpenGL && m_pWrappedOpenGL->m_State == WRITING_IDLE)
  {
    RenderDoc::Inst().SetCurrentDriver(RDC_VRAPI);

    m_AppControlledCapture = true;

    m_pWrappedOpenGL->StartFrameCapture(dev, wnd);
  }
}
bool WrappedVRAPI::EndFrameCapture(void *dev, void *wnd)
{
  bool bSuccessfulCapture = true;

  if(m_pWrappedOpenGL)
  {
    bSuccessfulCapture = m_pWrappedOpenGL->EndFrameCapture(dev, wnd);
  }

  return bSuccessfulCapture;
}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::SetWrappedOpenGL(WrappedOpenGL *wrappedOpenGL)
{
  m_pWrappedOpenGL = wrappedOpenGL;
}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::Common_CreateTextureSwapChain(GLuint texture_handle, ovrTextureFormat format,
                                                 int width, int height, int levels)
{
  //-----------------------------------------------------------------------------------------------
  // For OpenGLES
  //-----------------------------------------------------------------------------------------------
  if(m_pWrappedOpenGL)
  {
    void *gles_context = m_pWrappedOpenGL->GetCtx();
    GLResourceManager *gles_resource_manager = m_pWrappedOpenGL->GetResourceManager();
    Serialiser *gles_serialiser = m_pWrappedOpenGL->GetSerialiser();

    GLResource res = TextureRes(gles_context, texture_handle);
    ResourceId id = gles_resource_manager->RegisterResource(res);

    // I am just pretending vrapi_CreateTextureSwapChain is equivalent to glGenTexture for now
    // because I don't intend to playback the exact vrapi calls yet
    if(m_pWrappedOpenGL->m_State >= WRITING)
    {
      Chunk *chunk = NULL;

      {
        ScopedContext scope(gles_serialiser, m_pWrappedOpenGL->GetChunkName(GEN_TEXTURE),
                            GEN_TEXTURE, false);

        m_pWrappedOpenGL->Serialise_glGenTextures(1, &texture_handle);

        chunk = scope.Get();
      }

      GLResourceRecord *record = gles_resource_manager->AddResourceRecord(id);
      RDCASSERT(record);

      record->AddChunk(chunk);
    }
    else
    {
      gles_resource_manager->AddLiveResource(id, res);
    }

    GLenum curType = eGL_TEXTURE_2D;
    GLenum internalFormat = eGL_RGBA8;
    GLenum glformat = eGL_RGBA;
    GLenum type = eGL_UNSIGNED_BYTE;
    TextureCategory creationFlags = TextureCategory::ColorTarget | TextureCategory::SwapBuffer;

    switch(format)
    {
      case VRAPI_TEXTURE_FORMAT_565: internalFormat = eGL_RGB565; break;
      case VRAPI_TEXTURE_FORMAT_5551: internalFormat = eGL_RGB5_A1; break;
      case VRAPI_TEXTURE_FORMAT_4444: internalFormat = eGL_RGBA4; break;
      case VRAPI_TEXTURE_FORMAT_8888: internalFormat = eGL_RGBA8; break;
      case VRAPI_TEXTURE_FORMAT_8888_sRGB: internalFormat = eGL_SRGB8_ALPHA8; break;
      case VRAPI_TEXTURE_FORMAT_RGBA16F: internalFormat = eGL_RGBA16F; break;
      case VRAPI_TEXTURE_FORMAT_DEPTH_16:
        internalFormat = eGL_DEPTH_COMPONENT16;
        creationFlags = TextureCategory::DepthTarget;
        break;
      case VRAPI_TEXTURE_FORMAT_DEPTH_24:
        internalFormat = eGL_DEPTH_COMPONENT24;
        creationFlags = TextureCategory::DepthTarget;
        break;
      case VRAPI_TEXTURE_FORMAT_DEPTH_24_STENCIL_8:
        internalFormat = eGL_DEPTH24_STENCIL8;
        creationFlags = TextureCategory::DepthTarget;
        break;
      default: break;
    };

    m_pWrappedOpenGL->m_Textures[id].resource = res;
    m_pWrappedOpenGL->m_Textures[id].curType = curType;
    m_pWrappedOpenGL->m_Textures[id].dimension = 2;
    m_pWrappedOpenGL->m_Textures[id].width = width;
    m_pWrappedOpenGL->m_Textures[id].height = height;
    m_pWrappedOpenGL->m_Textures[id].depth = 1;
    m_pWrappedOpenGL->m_Textures[id].samples = 1;
    m_pWrappedOpenGL->m_Textures[id].creationFlags = creationFlags;
    m_pWrappedOpenGL->m_Textures[id].internalFormat = internalFormat;

    m_pWrappedOpenGL->Common_glTextureImage2DEXT(id, curType, 0, internalFormat, width, height, 0,
                                                 glformat, type, nullptr);
  }

  //-----------------------------------------------------------------------------------------------
  // TODO: Vulcan
  //-----------------------------------------------------------------------------------------------
}

void WrappedVRAPI::SubmitFrame(void *ovrMobile)
{
  LogState logstate = WRITING_IDLE;
  uint32_t frame_counter = 0;

  if(m_pWrappedOpenGL)
  {
    logstate = m_pWrappedOpenGL->m_State;
    frame_counter = m_pWrappedOpenGL->m_FrameCounter;
  }

  if(logstate == WRITING_IDLE)
    RenderDoc::Inst().Tick();

  RenderDoc::Inst().SetCurrentDriver(RDC_VRAPI);

  // kill any current capture that isn't application defined
  if(logstate == WRITING_CAPFRAME && !m_AppControlledCapture)
    RenderDoc::Inst().EndFrameCapture(ovrMobile, nullptr);

  if(RenderDoc::Inst().ShouldTriggerCapture(frame_counter) && logstate == WRITING_IDLE)
  {
    RenderDoc::Inst().StartFrameCapture(ovrMobile, nullptr);

    m_AppControlledCapture = false;
  }
}
