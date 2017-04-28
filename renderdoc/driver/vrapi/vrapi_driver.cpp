#include "vrapi_driver.h"
#include "driver/gl/gl_driver.h"


//------------------------------------------------------------------------------------------------------
WrappedVRAPI::WrappedVRAPI() :
	m_pWrappedOpenGL(nullptr),
	m_State(RenderDoc::Inst().IsReplayApp() ? READING : WRITING_IDLE),
	m_FrameCounter(0),
	m_AppControlledCapture(false)
{
	if (RenderDoc::Inst().GetCrashHandler())
		RenderDoc::Inst().GetCrashHandler()->RegisterMemoryRegion(this, sizeof(WrappedVRAPI));
}
WrappedVRAPI::~WrappedVRAPI()
{

}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::StartFrameCapture(void *dev, void *wnd)
{
	if (m_State != WRITING_IDLE)
		return;

	RenderDoc::Inst().SetCurrentDriver(RDC_VRAPI);

	m_State = WRITING_CAPFRAME;

	m_AppControlledCapture = true;

	if (m_pWrappedOpenGL)
	{
		m_pWrappedOpenGL->StartFrameCapture(dev, wnd);
	}
}
bool WrappedVRAPI::EndFrameCapture(void *dev, void *wnd)
{
	bool bSuccessfulCapture = true;

	if (m_pWrappedOpenGL)
	{
		bSuccessfulCapture = m_pWrappedOpenGL->EndFrameCapture(dev, wnd);

		m_State = m_pWrappedOpenGL->GetState();
	}
	else
	{
		m_State = WRITING_IDLE;
	}

	return bSuccessfulCapture;
}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::SetWrappedOpenGL(WrappedOpenGL * wrappedOpenGL)
{
	m_pWrappedOpenGL = wrappedOpenGL;
}

//------------------------------------------------------------------------------------------------------
void WrappedVRAPI::SubmitFrame( void * ovrMobile )
{
	if (m_State == WRITING_IDLE)
		RenderDoc::Inst().Tick();

	m_FrameCounter++;

	/*if (m_State == WRITING_IDLE)
	{
		uint32_t overlay = RenderDoc::Inst().GetOverlayBits();

		if (overlay & eRENDERDOC_Overlay_Enabled)
		{
			RenderTextState textState;

			textState.Push(m_Real, ctxdata.Modern());

			int flags = RenderDoc::eOverlay_ActiveWindow;
			if (ctxdata.Legacy())
				flags |= RenderDoc::eOverlay_CaptureDisabled;
			string overlayText = RenderDoc::Inst().GetOverlayText(GetDriverType(), m_FrameCounter, flags);

			if (ctxdata.Legacy())
			{
				if (!ctxdata.attribsCreate)
					overlayText += "Context not created via CreateContextAttribs. Capturing disabled.\n";
				overlayText += "Only OpenGL 3.2+ contexts are supported.\n";
			}
			else if (!ctxdata.isCore)
			{
				overlayText += "WARNING: Non-core context in use. Compatibility profile not supported.\n";
			}

			if (m_FailedFrame > 0)
			{
				const char *reasonString = "Unknown reason";
				switch (m_FailedReason)
				{
				case CaptureFailed_UncappedUnmap: reasonString = "Uncapped Map()/Unmap()"; break;
				default: break;
				}

				overlayText += StringFormat::Fmt("Failed capture at frame %d:\n", m_FailedFrame);
				overlayText += StringFormat::Fmt("    %s\n", reasonString);
			}

			if (!overlayText.empty())
				RenderOverlayText(0.0f, 0.0f, overlayText.c_str());

			textState.Pop(m_Real, ctxdata.Modern());

			// swallow all errors we might have inadvertantly caused. This is
			// better than letting an error propagate and maybe screw up the
			// app (although it means we might swallow an error from before the
			// SwapBuffers call, it can't be helped.
			if (ctxdata.Legacy() && m_Real.glGetError)
				ClearGLErrors(m_Real);
		}
	}*/

	/*if (m_State == WRITING_CAPFRAME && m_AppControlledCapture)
		m_BackbufferImages = SaveBackbufferImage();*/

	RenderDoc::Inst().SetCurrentDriver(RDC_VRAPI);

	// kill any current capture that isn't application defined
	if (m_State == WRITING_CAPFRAME && !m_AppControlledCapture)
		RenderDoc::Inst().EndFrameCapture(ovrMobile, nullptr);

	if (RenderDoc::Inst().ShouldTriggerCapture(m_FrameCounter) && m_State == WRITING_IDLE)
	{
		RenderDoc::Inst().StartFrameCapture(ovrMobile, nullptr);

		m_AppControlledCapture = false;
	}
}
