
#pragma once

#include "cinder/app/Renderer.h"

#if defined(CINDER_MSW)
	#if ! defined(NOMINMAX)
		#define NOMINMAX
	#endif
	#if ! defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <Windows.h>
#endif

namespace cinder { namespace app {

class TinyRenderer : public Renderer {
public:
	TinyRenderer() {}
	virtual ~TinyRenderer() {}

	virtual RendererRef	clone() const override { 
		return RendererRef( new TinyRenderer( *this ) ); 
	}

#if defined(CINDER_MSW)
	virtual void setup( HWND wnd, HDC dc, RendererRef sharedRenderer ) override {
		mHinstnace = ::GetModuleHandle( NULL );
		mHwnd = wnd;
	}

	virtual HWND getHwnd() override { 
		return mHwnd; 
	}

	virtual HINSTANCE getHinstance() const {
		return mHinstnace;
	}
#endif

	virtual void prepareToggleFullScreen() override {
	}

	virtual void finishToggleFullScreen() override {
	}

	virtual void kill() override {
	}

	virtual Surface8u copyWindowSurface( const Area &area, int32_t windowHeightPixels ) override {
		Surface8u result;
		return result;
	}

private:
	HINSTANCE		mHinstnace = nullptr;
	HWND			mHwnd = nullptr;
};

}} // namespace cinder::app