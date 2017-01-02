
#pragma once

#include "cinder/app/Renderer.h"

#if defined(TINY_RENDERER_DX)
	#include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
	#include "tinyvk.h"
#endif

namespace cinder { namespace app {

class TinyRenderer : public Renderer {
public:
	class Options {
	public:

	private:
		friend class TinyRenderer;

	};

	TinyRenderer( const Options& options = Options() ) : mOptions(options) {
	}

	virtual ~TinyRenderer() {
	}

	virtual RendererRef	clone() const override { 
		return RendererRef( new TinyRenderer( *this ) ); 
	}

#if defined( CINDER_MSW )
	virtual void setup( HWND wnd, HDC dc, RendererRef sharedRenderer ) override {
		mWnd = wnd;

		::RECT clientRect;
		::GetClientRect( mWnd, &clientRect );
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;

		tr_renderer_settings settings = {0};
		settings.handle.hinstance               = ::GetModuleHandle( NULL );
		settings.handle.hwnd                    = mWnd;
		settings.width                          = width;
		settings.height                         = height;
		settings.swapchain.image_count          = 2;
		settings.swapchain.sample_count         = tr_sample_count_1;
		settings.swapchain.color_format         = tr_format_b8g8r8a8_unorm;
		settings.swapchain.depth_stencil_format = tr_format_d16_unorm;
		tr_result res = tr_create_renderer("myapp", &settings, &mRenderer);
		assert(tr_result_ok == res);
	}

	virtual HWND getHwnd() override { 
		return mWnd; 
	}
#endif

	virtual void prepareToggleFullScreen() override {
	}

	virtual void finishToggleFullScreen() override {
	}

	virtual void kill() override {
		if (nullptr != mRenderer) {
			tr_result res = tr_destroy_renderer(mRenderer);
			assert(tr_result_ok == res);
		}
	}

	virtual Surface8u copyWindowSurface( const Area &area, int32_t windowHeightPixels ) override {
		Surface8u result;
		return result;
	}

	tr_renderer* const getRenderer() const {
		return mRenderer;
	}

private:
	Options			mOptions;	
	HWND			mWnd = nullptr;
	tr_renderer*	mRenderer = nullptr;
};

}} // namespace cinder::app