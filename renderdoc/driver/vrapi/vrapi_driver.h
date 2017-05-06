/******************************************************************************
* The MIT License (MIT)
*
* Copyright (c) 2017 Jimmy Lee
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


#pragma once

#include "core/core.h"
#include "driver/vrapi/official/VrApi_Types.h"
#include "driver/gl/official/glcorearb.h"




class WrappedOpenGL;


class WrappedVRAPI : public IFrameCapturer
{
public:

	WrappedVRAPI();
	~WrappedVRAPI();

	void      StartFrameCapture(void *dev, void *wnd);
	bool      EndFrameCapture(void *dev, void *wnd);

	void      SetWrappedOpenGL(WrappedOpenGL * wrappedOpenGL);
	
	//-------------------------------------------------------------------------
  void      Common_CreateTextureSwapChain(GLuint texture_handle, ovrTextureFormat format, int width, int height, int levels);
	void      SubmitFrame(void * ovrMobile);

private:
	WrappedOpenGL * m_pWrappedOpenGL;
  //TODO: WrappedVulkan * m_pWrappedVulkan;
  
	LogState    m_State;
	uint32_t    m_FrameCounter;

	bool        m_AppControlledCapture;
};