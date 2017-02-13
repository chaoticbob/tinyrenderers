/*

Copyright (c) 2016, Libertus Code
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
  * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TINYCI_H
#define TINYCI_H

#if defined(_WIN32)
  #define CINDER_MSW
  #if ! defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
  #endif
  #if ! defined(NOMINMAX)
    #define NOMINMAX
  #endif
  #include <Windows.h>
  #include <comutil.h>
  #pragma comment(lib, "comsuppw.lib")
#elif defined(__linux__)
  #define CINDER_LINUX
#endif

#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// -------------------------------------------------------------------------------------------------
// Support classes
// -------------------------------------------------------------------------------------------------
namespace cinder {
  class Buffer;
  class DataSource;
  class ImageSource;
  class ImageTarget;
  using BufferRef = std::shared_ptr<Buffer>;
  using DataSourceRef = std::shared_ptr<DataSource>;
  using ImageSourceRef = std::shared_ptr<ImageSource>;
  using ImageTargetRef = std::shared_ptr<ImageTarget>;

  namespace fs {
    class path;
  } // namespace fs

  //! \class Buffer
  //!
  //!
  class Buffer {
  public:
    Buffer();
    Buffer(size_t n);
    virtual ~Buffer();
    void            setSize(size_t n);
    size_t          getSize() const;
    uint8_t*        getData();
    const uint8_t*  getData() const;
  private:
    std::vector<uint8_t>  mData;
  };

  //! \class DataSource
  //!
  //!
  class DataSource {
  public:
    DataSource();
    DataSource(const BufferRef& buffer);
    DataSource(const fs::path& path);
    virtual ~DataSource();
    BufferRef getBuffer() const;
  protected:
    BufferRef mBuffer;
    bool      mFilePath = false;
  };

  //! \class Area 
  //!
  //!
  class Area {
  };

  //! \class ImageSource
  //!
  //!
  class ImageSource {
  public:
    ImageSource();
    ImageSource(uint32_t width, uint32_t height, uint32_t channelCount, uint32_t rowBytes, const BufferRef& buffer);
    virtual ~ImageSource();
    virtual void      load(ImageTargetRef& target);
    int32_t           getWidth() const;
    int32_t           getHeight() const;
    bool              hasAlpha() const;
    int32_t           getRowBytes() const;
    const BufferRef&  getBuffer() const;
  private:
    uint32_t  mWidth = 0;
    uint32_t  mHeight = 0;
    uint32_t  mChannelCount = 0;
    uint32_t  mRowBytes = 0;
    BufferRef mBuffer;
  };

  //! \class SurfaceT
  //!
  //!
  template <typename T> class SurfaceT {
  public:
    SurfaceT();
    SurfaceT(const ImageSourceRef& imageSource);
    virtual ~SurfaceT();
    int32_t               getWidth() const;
    int32_t               getHeight() const;
    bool                  hasAlpha() const;
    int32_t               getRowBytes() const;
    T*                    getData();
    const T*              getData() const;
  private:
    int32_t               mWidth = 0;
    int32_t               mHeight = 0;
    bool                  mHasAlpha = false;
    int32_t               mRowBytes = 0;
    BufferRef             mStorage;
    T*                    mExternalData = nullptr;
  };

  using Surface8u     = SurfaceT<uint8_t>;
  using Surface32f    = SurfaceT<float>;
  using Surface       = SurfaceT<uint8_t>;
  using Surface8uRef  = std::shared_ptr<Surface8u>;
  using Surface32fRef = std::shared_ptr<Surface32f>;
  using SurfaceRef    = std::shared_ptr<Surface>;
} // namespace cinder

// -------------------------------------------------------------------------------------------------
// Core classes
// -------------------------------------------------------------------------------------------------
namespace cinder {
	//! \class Timer
	//!
	//!
  class Timer {
  public:
	  //! Constructs a default timer which is initialized as stopped
	  Timer();
	  //! Constructs a default timer which is initialized as running unless \a startOnConstruction is false
	  Timer( bool startOnConstruction );
	  //! Begins timing. Optional \a offsetSeconds parameter allows a relative offset
	  void	  start( double offsetSeconds = 0 );
	  //! Returns the elapsed seconds if the timer is running, or the total time between calls to start() and stop() if it is stopped.
	  double	getSeconds() const;
	  //! Ends timing
	  void	  stop();
	  //! Resumes timing without resetting the timer.
	  void	  resume() { start( getSeconds() ); }
	  //! Returns whether the timer is currently running
	  bool	  isStopped() const { return mIsStopped; }
  private:
	  bool	  mIsStopped = true;
	  double	mStartTime, mEndTime, mInvNativeFreq;
  };

  namespace fs {
    class path {
    public:
      path() {}
      path(const std::string& s) { append(s, true); }
      path(const char* s) { append(s, true); }
      path(const path& p) { m_cached = p.m_cached; m_dirty = p.m_dirty;
        m_has_root = p.m_has_root; m_parts = p.m_parts; }
      virtual ~path() {}
      operator bool() const { update_cache(); return m_cached.empty() ? false : true; }
      operator std::string() const { return str(); }
      bool  operator==(const path& rhs) const { return str() == rhs.str(); }
      bool  operator!=(const path& rhs) const { return str() != rhs.str(); }
      path& operator=(const path& rhs) { if (this != &rhs) { m_cached = rhs.m_cached;
                m_dirty = rhs.m_dirty; m_has_root = rhs.m_has_root; m_parts = rhs.m_parts; } return *this; }
      path& operator=(const std::string& rhs) { append(rhs, true); return *this; }
      path& operator=(const char* rhs) { append(rhs, true); return *this; }
      path& operator/=(const path& rhs) { append(rhs.str()); return *this; }
      path& operator/=(const std::string& rhs) { append(rhs); return *this; }
      path& operator/=(const char* rhs) { append(rhs); return *this; }
      path  operator/(const path& rhs) const { path r = *this; r.append(rhs.str()); return r; }
      path  operator/(const std::string& rhs) const { path r = *this; r.append(rhs); return r; }
      path  operator/(const char* rhs) const { path r = *this; r.append(rhs); return r; }
      const std::string&  str() const { update_cache(); return m_cached; }
      const char* c_str() const { update_cache(); return m_cached.c_str(); }
      bool  is_root() const { update_cache(); 
                return ((m_cached.size() == 1) && (m_cached[0] == '/')) || 
                       ((m_cached.size() == 2) && (m_cached[1] == ':'));}
      fs::path  parent() const { fs::path r = *this; 
                     if ((! r.is_root()) && (! r.m_parts.empty())) { r.m_parts.pop_back();}
                    r.m_cached.clear(); r.m_dirty = true; return r; }
    private:
      void append(std::string s, bool reset = false) {
        if (s.empty()) { return; }
        if ((! reset) && (s[0] == '/')) {
          throw std::runtime_error("cannot append path that contains root");
        }
        // Change all '\' to '/'
        if (s.find('\\') != std::string::npos) {
          std::transform(s.begin(), s.end(), s.begin(),
                         [](typename std::string::value_type c) {
                         return ( c == '\\' ) ? '/' : c; });
        }
        // Collapse repeating '/' to a single '/'
        while (s.find("//") != std::string::npos) {
          auto new_end = std::unique(s.begin(), s.end(), 
                                     [](typename std::string::value_type lhs, 
                                        typename std::string::value_type rhs) 
                                     { return (lhs == rhs) && (lhs == '/'); });
          s.erase(new_end, s.end());
        }
        // Clear everything if this is a reset
        if (reset) { m_cached.clear(); m_dirty = false; m_has_root = false;  m_parts.clear(); }
        // Split string into components
        m_has_root = (! s.empty()) && ((s[0] == '/') || ((s.size() > 1) && (s[1] == ':')));
        if (s.find('/') != std::string::npos) {
          std::istringstream is(s);
          while (std::getline(is, s, '/')) { m_parts.push_back(s); }
        } else {
          m_parts.push_back(s);
        }
        // Mark dirty
        m_dirty = true;
      }
      void update_cache() const {
        if (! m_dirty) { return; }
        m_cached.clear();
        if (m_has_root && (! ((m_parts[0].size() > 1) && (m_parts[0][1] == ':')))) { 
            m_cached = "/"; 
        }
        if (! m_parts.empty()) {
          auto iter = m_parts.begin();
          m_cached = *(iter++);
          while (iter != m_parts.end()) {
            m_cached += '/';
            m_cached += *(iter++);
          }
        }
        m_dirty = false;
      }
    private:
      mutable std::string       m_cached;
      mutable bool              m_dirty = false;
      mutable bool              m_has_root = false;
      std::vector<std::string>  m_parts;
    };

    bool  exists(const fs::path& p);
    bool  is_file(const fs::path& p);
    bool  is_directory(const fs::path& p);
  } // namespace fs

  namespace app {
    class App;
	  class Window;
	  class Renderer;
    using RendererRef = std::shared_ptr<Renderer>;
    using WindowRef = std::shared_ptr<Window>;

    namespace detail {
      using CreateRendererFn = std::function<cinder::app::Renderer*()>;
	    struct AppParams {
        std::string       name;
#if defined(CINDER_MSW)
		    HINSTANCE			    hInstance = nullptr;
        int               nCmdShow = 0;
#endif
		    CreateRendererFn  createRendererFn;
	    };
	  } // namespace detail

	  //! \class KeyEvent
	  //!
	  //!
    class KeyEvent {
    };

	  //! \class MouseEvent
	  //!
	  //!
    class MouseEvent {
    };

	  //! \class WindowImpl
	  //!
	  //!
	  class WindowImpl {
	  public:
      WindowImpl(Window* window, int32_t width, int32_t height);
      virtual ~WindowImpl();
      RendererRef   getRenderer() const;
      int32_t       getWidth() const { return mWidth; }
      int32_t       getHeight() const { return mHeight; }
      virtual void  redraw() = 0;
    protected:
      virtual void  setupRenderer() = 0;
	  protected:
	    Window*			  mWindow;
	    RendererRef	  mRenderer;
      int32_t       mWidth = 0;
      int32_t       mHeight = 0;
    private:
      void createRenderer(detail::CreateRendererFn createRendererFn);      
      friend class Window;
	  };
    
	  //! \class Window
	  //!
	  //!
	  class Window : public std::enable_shared_from_this<Window> {
	  public:
      Window(App* app, int32_t width, int32_t height);
      virtual ~Window();
      App*          getApp() const {return mApp;}
      RendererRef   getRenderer() const;
      int32_t       getWidth() const { return mImpl->getWidth(); }
      int32_t       getHeight() const { return mImpl->getHeight(); }
      virtual void  close();
      virtual void  redraw();
	  private:
      App*                        mApp = nullptr;
	    std::unique_ptr<WindowImpl>	mImpl;
    public:
      //! Do not call this function direct, the behavior is undefined.
      void private_draw();
	  };

    //! \class AppImpl
    //!
    //!
    class AppImpl {
    public:
      AppImpl(App* app);
      virtual ~AppImpl();
      virtual fs::path  getAppPath() const = 0;
      WindowRef         getWindow() const { return mCurrentWindow; }
      void              setWindow(const WindowRef& window) { mCurrentWindow = window; }
      uint64_t          getElapsedFrames() const { return mElapsedFrames; }
      double            getElapsedSeconds() const;
      void              enableFrameRate(bool value = true);
      void              disableFrameRate();
      void              setFrameRate(float value);
      virtual fs::path  getAssetPath(const fs::path& relativePath) const;
      virtual void      addAssetDirectory(const fs::path& directory);
    protected:
      virtual void  run() = 0;
      virtual void  closeWindow(const WindowRef& window) = 0;
      friend class App;
    protected:
      App*						        mApp = nullptr;
      bool                    mShouldQuit = false;
	    std::vector<WindowRef>  mWindows;
      WindowRef               mCurrentWindow;
      uint64_t                mElapsedFrames = 0;
      Timer                   mTimer;
      bool                    mFrameRateEnabled = true;
      float                   mFrameRate = 60.0f;
      std::vector<fs::path>   mAssetDirs;
    private:
      void private_redraw();
    };
    
    //! \class App
    //!
    //!
    class App {
    public:
      class Settings {
      public:
        Settings() {}
        virtual ~Settings() {}
        int32_t   getWindowWidth() const;
        int32_t   getWindowHeight() const;
        void      setWindowSize(int32_t width, int32_t height);
        float     getFrameRate() const;
        void      setFrameRate(float value);
      private:
        int32_t   mWindowWidth = 640;
        int32_t   mWindowHeight = 480;
        float     mFrameRate = 60.0f;
        friend class App;
      };

      using SettingsFn = std::function<void(App::Settings*)>;

      App();
      virtual ~App();

      const detail::AppParams*  getAppParams() const;
      const App::Settings*      getSettings() const;

      // Do not call this function directly, behavior is undefined.
      void          private_run(); 

      fs::path      getAppPath() const { return mImpl->getAppPath(); }

      WindowRef     getWindow() const { return mImpl->getWindow(); }
      int32_t       getWindowWidth() const { return getWindow()->getWidth(); }
      int32_t       getWindowHeight() const { return getWindow()->getHeight(); }
      RendererRef   getRenderer() const { return getWindow()->getRenderer(); }

      fs::path      getAssetPath(const fs::path& relativePath) const { return mImpl->getAssetPath(relativePath); }
      void          addAssetDirectory(const fs::path& directory) { mImpl->addAssetDirectory(directory); }

      uint64_t      getElapsedFrames() const { return mImpl->getElapsedFrames(); }
      double        getElapsedSeconds() const { return mImpl->getElapsedSeconds(); }

      // Client app functions
      virtual void  setup() {}
      virtual void  cleanup() {}
      virtual void	mouseDown(MouseEvent event) {}
      virtual void	mouseUp(MouseEvent event) {}
      virtual void	mouseDrag(MouseEvent event) {}
      virtual void	keyDown(KeyEvent event) {}
      virtual void	update() {}
      virtual void	draw() {}
    private:
      std::unique_ptr<detail::AppParams>  mAppParams;
      std::unique_ptr<App::Settings>      mSettings;
      std::unique_ptr<AppImpl>	          mImpl;

    private:
      void private_setup();
      void private_update();
      void private_draw(const WindowRef& window);
      void private_redraw();
      void private_close_window(const WindowRef& window);
#if defined(CINDER_MSW)
      friend class AppImplMsw;
#endif
      friend class Window;

    public:
      static void configureApp(detail::AppParams* appParams,
                               App::Settings* settings,
                               App::SettingsFn prepareSettingsFn = nullptr);
    };

#if defined(CINDER_MSW)
    //! \class AppImplMsw
    //!
    //!
    class AppImplMsw : public AppImpl {
	  public:
      AppImplMsw(App* app);
      virtual ~AppImplMsw();
      virtual fs::path  getAppPath() const override;
    protected:
      virtual void  run() override;
      virtual void  closeWindow(const WindowRef& window) override;
    private:
      void          sleep( double seconds );
	  private:
      double  mNextFrameTime = 0;
    };

	  //! \class WindowImplMsw
	  //!
	  //!
	  class WindowImplMsw : public WindowImpl {	
	  public:
        WindowImplMsw(Window* window, int32_t width, int32_t height);
        virtual ~WindowImplMsw();
        virtual void  redraw() override;
      protected:
        virtual void  setupRenderer() override;
	  private:
        HWND                  mWnd = nullptr;
        HDC                   mDc = nullptr;
        std::vector<wchar_t>  mWindowClassName;
	  };
#elif defined(CINDER_LINUX)
	  //! \class App
	  //!
	  //!
    class AppImplLinux : public AppImpl {
    };

	  //! \class App
	  //!
	  //!
	  class WindowImplLinux : public WindowImpl {
	  };
#endif

	  //! \class Renderer
	  //!
	  //!
	  class Renderer {
	  public:
      Renderer();
      virtual ~Renderer();
      virtual RendererRef	clone() const = 0;
#if defined(CINDER_MSW)
      virtual void setup(HWND wnd, HDC dc, std::shared_ptr<Renderer> sharedRenderer) = 0;
      virtual HWND getHwnd() = 0;
#endif
      virtual void prepareToggleFullScreen() = 0;
      virtual void finishToggleFullScreen() = 0;
      virtual void kill() = 0;
      virtual Surface8u copyWindowSurface(const Area &area, int32_t windowHeightPixels) = 0;
	  };

	  //! \class RendererGl
	  //!
	  //!
    class RendererGl : public Renderer {
    public:
      RendererGl();
      virtual ~RendererGl();

	    virtual RendererRef	clone() const override { 
		    return RendererRef( new RendererGl( *this ) ); 
	    }

      virtual void  setup(HWND wnd, HDC dc, std::shared_ptr<Renderer> sharedRenderer) override;

	    virtual HWND getHwnd() override { 
		    return 0; 
	    }


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
    };
  } // namespace app
} // namespace cinder

// -------------------------------------------------------------------------------------------------
// Support functions
// -------------------------------------------------------------------------------------------------
namespace cinder {
  DataSourceRef   loadFile(const fs::path& path);
#if defined(LC_IMAGE_H)
  ImageSourceRef  loadImage(const fs::path& path);
#endif
} // namespace cinder

namespace ci = cinder;

// =================================================================================================
// IMPLEMENTATION
// =================================================================================================
#if defined(TINYCI_IMPLEMENTATION)
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <sys/types.h>
#include <sys/stat.h>

// -------------------------------------------------------------------------------------------------
// Support classes
// -------------------------------------------------------------------------------------------------
namespace cinder {
	// -----------------------------------------------------------------------------------------------
	// Buffer
	// -----------------------------------------------------------------------------------------------
  Buffer::Buffer() {}
  Buffer::Buffer(size_t n) { setSize(n); }
  Buffer::~Buffer() {}
  void Buffer::setSize(size_t n) { mData.resize(n); }
  size_t Buffer::getSize() const { return mData.size();  }
  uint8_t* Buffer::getData() { return mData.data(); }
  const uint8_t* Buffer::getData() const { return mData.data(); }

	// -----------------------------------------------------------------------------------------------
	// DataSource
	// -----------------------------------------------------------------------------------------------
  DataSource::DataSource() {}
  DataSource::DataSource(const BufferRef& buffer) : mBuffer(buffer) {}
  DataSource::DataSource(const fs::path& path) : mFilePath(true) {
    const auto& s = path.str();
    mBuffer = std::make_shared<Buffer>(s.size());
    std::copy(s.begin(), s.begin() + s.size(), mBuffer->getData());
  }
  DataSource::~DataSource() {}
  BufferRef DataSource::getBuffer() const { return mBuffer; }

	// -----------------------------------------------------------------------------------------------
	// ImageSource
	// -----------------------------------------------------------------------------------------------
  ImageSource::ImageSource() {} 
  ImageSource::ImageSource(uint32_t width, uint32_t height, 
                           uint32_t channelCount, uint32_t rowBytes, 
                           const BufferRef& buffer)
    : mWidth(width), mHeight(height), mChannelCount(channelCount), 
    mRowBytes(rowBytes), mBuffer(buffer) {}
  ImageSource::~ImageSource() {}
  void ImageSource::load(ImageTargetRef& target) { /* Do nothing for now */ }
  int32_t ImageSource::ImageSource::getWidth() const { return mWidth; }
  int32_t ImageSource::ImageSource::getHeight() const { return mHeight; }
  bool ImageSource::ImageSource::hasAlpha() const { return (mChannelCount == 4) ? true : false; }
  int32_t ImageSource::ImageSource::getRowBytes() const { return mRowBytes; }
  const BufferRef&  ImageSource::ImageSource::getBuffer() const { return mBuffer; }

	// -----------------------------------------------------------------------------------------------
	// SurfaceT
	// -----------------------------------------------------------------------------------------------
  template <typename T>
  SurfaceT<T>::SurfaceT() {}

  template <typename T>
  SurfaceT<T>::SurfaceT(const ImageSourceRef& imageSource) {
    if (! imageSource) {
      throw std::runtime_error("Invalid image source");
    }
    mWidth = imageSource->getWidth();
    mHeight = imageSource->getHeight();
    mHasAlpha = imageSource->hasAlpha();
    mRowBytes = imageSource->getRowBytes();
    mStorage = imageSource->getBuffer();
  }

  template <typename T>
  SurfaceT<T>::~SurfaceT() {}

  template <typename T>
  int32_t SurfaceT<T>::getWidth() const { return mWidth; }

  template <typename T>
  int32_t SurfaceT<T>::getHeight() const { return mHeight; }

  template <typename T>
  bool SurfaceT<T>::hasAlpha() const { return mHasAlpha; }

  template <typename T>
  int32_t SurfaceT<T>::getRowBytes() const { return mRowBytes; }

  template <typename T>
  T* SurfaceT<T>::getData() { 
    return (mExternalData != nullptr) ? mExternalData 
                                      : reinterpret_cast<T*>(mStorage->getData()); 
  }

  template <typename T>
  const T* SurfaceT<T>::getData() const { 
    return (mExternalData != nullptr) ? mExternalData 
                                      : reinterpret_cast<const T*>(mStorage->getData()); 
  }
} // namespace cinder

// -------------------------------------------------------------------------------------------------
// Core classes
// -------------------------------------------------------------------------------------------------
namespace cinder {
  Timer::Timer() {
#if defined( CINDER_COCOA ) || defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	  mEndTime = mStartTime = -1;
#elif defined( CINDER_MSW )
	  ::LARGE_INTEGER nativeFreq;
	  ::QueryPerformanceFrequency( &nativeFreq );
	  mInvNativeFreq = 1.0 / nativeFreq.QuadPart;
	  mStartTime = mEndTime = -1;
#endif
  }

  Timer::Timer( bool startOnConstruction ) {
#if defined( CINDER_COCOA ) || defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
		mEndTime = mStartTime = -1;
#elif defined( CINDER_MSW )
	  ::LARGE_INTEGER nativeFreq;
	  ::QueryPerformanceFrequency( &nativeFreq );
  	mInvNativeFreq = 1.0 / nativeFreq.QuadPart;
  	mStartTime = mEndTime = -1;
#endif
	  if( startOnConstruction ) { start(); }
  }

  void Timer::start(double offsetSeconds)
  {
#if defined( CINDER_COCOA )
	  mStartTime = ::CFAbsoluteTimeGetCurrent() - offsetSeconds;
#elif defined( CINDER_MSW )
    ::LARGE_INTEGER rawTime;
    ::QueryPerformanceCounter( &rawTime );
    mStartTime = rawTime.QuadPart * mInvNativeFreq - offsetSeconds;
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
	  mStartTime = LinuxGetElapsedSeconds();
#endif
  	mIsStopped = false;
  }

  double Timer::getSeconds() const
  {
	  if( mIsStopped ) {
		  return mEndTime - mStartTime;
    } else {
#if defined( CINDER_COCOA )
      return ::CFAbsoluteTimeGetCurrent() - mStartTime;
#elif defined( CINDER_MSW )
      ::LARGE_INTEGER rawTime;
      ::QueryPerformanceCounter( &rawTime );
      return (rawTime.QuadPart * mInvNativeFreq) - mStartTime;
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
      return LinuxGetElapsedSeconds() - mStartTime;
#endif
	  }
  }

  void Timer::stop() {
	  if( ! mIsStopped ) {
#if defined( CINDER_COCOA )
      mEndTime = ::CFAbsoluteTimeGetCurrent();
#elif defined( CINDER_MSW )
      ::LARGE_INTEGER rawTime;
      ::QueryPerformanceCounter( &rawTime );
      mEndTime = rawTime.QuadPart * mInvNativeFreq;
#elif defined( CINDER_ANDROID ) || defined( CINDER_LINUX )
      mEndTime = LinuxGetElapsedSeconds();
#endif
      mIsStopped = true;
	  }
  }

  namespace fs {
    bool exists(const fs::path& p) {
      struct stat info = {};
      return 0 == stat(p.c_str(), &info);
    }

    bool is_file(const fs::path& p) {
      struct stat info = {};
      if (0 != stat(p.c_str(), &info)) { return false;}
      return S_IFREG == (info.st_mode & S_IFREG);
    }

    bool is_directory(const fs::path& p) {
      struct stat info = {};
      if (0 != stat(p.c_str(), &info)) { return false;}
      return S_IFDIR == (info.st_mode & S_IFDIR);
    }
  } // namespace fs

  namespace app {
    namespace detail {
      // Do not use these directly, used by App for temp values
	    static std::unique_ptr<AppParams>     sAppParams;
      static std::unique_ptr<App::Settings> sAppSettings;
	  } // namespace detail

    // ---------------------------------------------------------------------------------------------
	  // App::Settings
	  // ---------------------------------------------------------------------------------------------
    int32_t App::Settings::getWindowWidth() const
    {
      return mWindowWidth;
    }

    int32_t App::Settings::getWindowHeight() const
    {
      return mWindowHeight;
    }

    void App::Settings::setWindowSize(int32_t width, int32_t height) {
      mWindowWidth = width;
      mWindowHeight = height;
    }

    float App::Settings::getFrameRate() const {
      return mFrameRate;
    }

    void App::Settings::setFrameRate(float value) {
      mFrameRate = std::max(0.00001f, value);
    }

	  // ---------------------------------------------------------------------------------------------
	  // App
	  // ---------------------------------------------------------------------------------------------
    App::App() {
      mAppParams = std::move(detail::sAppParams);
      mSettings = std::move(detail::sAppSettings);
#if defined(CINDER_MSW)
	    mImpl = std::make_unique<AppImplMsw>(this);
#endif
      // Start with full path to executable
      fs::path dir = getAppPath();
      while (dir) {
        dir = dir.parent();
        fs::path assetDir = dir / "assets";
        mImpl->addAssetDirectory(assetDir);
        if (dir.is_root()) {
          break;
        }
      }
	  }

    App::~App() {
	  }

    const detail::AppParams* App::getAppParams() const
    {
      return mAppParams.get();
    }

    const App::Settings* App::getSettings() const
    {
      return mSettings.get();
    }

    void App::private_run() {
      mImpl->run();
    }

    void App::private_setup() {
      setup();
    }

    void App::private_update() {
      update();
    }

    void App::private_draw(const WindowRef& window)
    {
      mImpl->setWindow(window);
      draw();
    }

    void App::private_redraw() {
      mImpl->private_redraw();
    }

    void App::private_close_window(const WindowRef& window)
    {
      mImpl->closeWindow(window);
    }

    void App::configureApp(detail::AppParams* appParams, 
                           App::Settings* settings,
                           App::SettingsFn prepareSettingsFn)
    {
      detail::sAppParams = std::unique_ptr<detail::AppParams>(appParams);
      detail::sAppSettings = std::unique_ptr<App::Settings>(settings);
      if (prepareSettingsFn) {
        prepareSettingsFn(detail::sAppSettings.get());
      }
    }

	  // ---------------------------------------------------------------------------------------------
	  // AppImpl
	  // ---------------------------------------------------------------------------------------------
    AppImpl::AppImpl(App* app)
	  	: mApp(app)
	  {
	  }
	  
	  AppImpl::~AppImpl() {
	  }

    double AppImpl::getElapsedSeconds() const
    {
      return mTimer.getSeconds();
    }

    void AppImpl::enableFrameRate(bool value) { 
      mFrameRateEnabled = value;
    }
    
    void AppImpl::disableFrameRate() {
      enableFrameRate(false);
    }
    
    void AppImpl::setFrameRate(float value) { 
      mFrameRate = std::max(0.00001f, value); 
    }

    fs::path AppImpl::getAssetPath(const fs::path& relativePath) const
    {
      fs::path result;
      for (const auto& dir : mAssetDirs) {
        fs::path p = dir / relativePath;
        if (fs::exists(p)) {
          result = p;
          break;
        }
      }
      return result;
    }

    void AppImpl::addAssetDirectory(const fs::path& directory)
    {
      auto it = std::find_if(std::begin(mAssetDirs),
                             std::end(mAssetDirs),
                             [&directory](const fs::path& elem) -> bool {
                             return directory == elem; });
      if (it == std::end(mAssetDirs)) {
        mAssetDirs.push_back(directory);
      }
    }

    void AppImpl::private_redraw()
    {
      for (auto& window : mWindows) {
        window->redraw();
      }
    }

	  // ---------------------------------------------------------------------------------------------
	  // Window
	  // ---------------------------------------------------------------------------------------------
    Window::Window(App* app, int32_t width, int32_t height)
      : mApp(app)
    {
#if defined(CINDER_MSW)
	    mImpl = std::make_unique<WindowImplMsw>(this, width, height);
#endif
      auto createRendererFn = app->getAppParams()->createRendererFn;
      mImpl->createRenderer(createRendererFn);
      mImpl->setupRenderer();
	  }

    Window::~Window() {
	  }

    RendererRef Window::getRenderer() const
    {
      return mImpl->getRenderer();
    }

    void Window::close()
    {
      mApp->private_close_window(shared_from_this());
    }

    void Window::redraw()
    {
      mImpl->redraw();
    }

    void Window::private_draw() {
      mApp->private_draw(shared_from_this());
    }

	  // ---------------------------------------------------------------------------------------------
	  // WindowImpl
	  // ---------------------------------------------------------------------------------------------
    WindowImpl::WindowImpl(Window* window, int32_t width, int32_t height)
      : mWindow(window), mWidth(width), mHeight(height)
    {
    }
    
    WindowImpl::~WindowImpl() {}

    RendererRef WindowImpl::getRenderer() const
    {
      return mRenderer;
    }

    void WindowImpl::createRenderer(detail::CreateRendererFn createRendererFn) {
      Renderer* renderer = createRendererFn();
      mRenderer = RendererRef(renderer);
    }

	  // ---------------------------------------------------------------------------------------------
	  // Renderer
	  // ---------------------------------------------------------------------------------------------
    Renderer::Renderer() {}

    Renderer::~Renderer() {}

	  // ------------------------------------------------------------------------------------------------
	  // RendererGl
	  // ------------------------------------------------------------------------------------------------
    RendererGl::RendererGl() {}

    RendererGl::~RendererGl() {}

    void RendererGl::setup(HWND wnd, HDC dc, std::shared_ptr<Renderer> sharedRenderer) {
      int stopMe = 1;
    }
  } // namespace app
} // namespace cinder

#if defined(CINDER_MSW)
namespace cinder {
  namespace app {
	  // ---------------------------------------------------------------------------------------------
	  // AppImplMsw
	  // ---------------------------------------------------------------------------------------------
    AppImplMsw::AppImplMsw(App* app)
      : AppImpl(app) 
	  {              
        int32_t width = mApp->getSettings()->getWindowWidth();
        int32_t height = mApp->getSettings()->getWindowHeight();
        auto window = std::make_shared<Window>(app, width, height);
        mWindows.push_back(window);
        setWindow(window);
	  }

    AppImplMsw::~AppImplMsw() {
	  }

    fs::path AppImplMsw::getAppPath() const {
      HMODULE module = GetModuleHandle(NULL);
      WCHAR buf16[MAX_PATH] = {};
      DWORD size = GetModuleFileName(module, buf16, MAX_PATH);

      size_t numConverted = 0;
      std::vector<char> buf8(size + 1);
      std::fill(std::begin(buf8), std::end(buf8), 0);
      wcstombs_s(&numConverted, buf8.data(), size + 1, buf16, size);

      fs::path result = fs::path((const char*)buf8.data());
      return result;
    }

    void AppImplMsw::run() {
      mTimer.start();

      mApp->private_setup();

      mNextFrameTime = getElapsedSeconds();
      
      while (! mShouldQuit) {
        // Regulate frame rate
        mApp->private_update();
        if (! mShouldQuit ) {
          mApp->private_redraw();
          ++mElapsedFrames;
        }

        // get current time in seconds
        double currentSeconds = mApp->getElapsedSeconds();
        // calculate time per frame in seconds
        double secondsPerFrame = 1.0 / mFrameRate;
        // determine when next frame should be drawn
        mNextFrameTime += secondsPerFrame;

        // sleep and process messages until next frame
        if ((mFrameRateEnabled) && (mNextFrameTime > currentSeconds)) {
          sleep(mNextFrameTime - currentSeconds);
        } else {
          MSG msg;
          while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
          }
        }
      }
    }

    void AppImplMsw::sleep(double seconds) {
      // create waitable timer
      static HANDLE timer = ::CreateWaitableTimer(NULL, FALSE, NULL);
      // specify relative wait time in units of 100 nanoseconds
      LARGE_INTEGER waitTime;
      waitTime.QuadPart = (LONGLONG)(seconds * -10000000);
      if(waitTime.QuadPart >= 0) return;
      // activate waitable timer
      if (!::SetWaitableTimer(timer, &waitTime, 0, NULL, NULL, FALSE)) {
        return;
      }

      // handle events until specified time has elapsed
      DWORD result;
      MSG msg;
      while (! mShouldQuit) {
        result = ::MsgWaitForMultipleObjects(1, &timer, false, INFINITE, QS_ALLINPUT);
        if (result == (WAIT_OBJECT_0 + 1)) {
          // execute messages as soon as they arrive
          while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
          }
          // resume waiting
        }
        else { 
          return; // time has elapsed
        }
      }
    }

    void AppImplMsw::closeWindow(const WindowRef& window)
    {
      mWindows.erase(
          std::remove_if(std::begin(mWindows), std::end(mWindows),
                         [&window](const auto& elem) -> bool {
                              return window == elem; }),
          std::end(mWindows));
      if (mWindows.empty()) {
        mShouldQuit = true;
      }
    }

	  // ---------------------------------------------------------------------------------------------
	  // WindowImplMsw
	  // ---------------------------------------------------------------------------------------------
    static std::unordered_map<HWND, Window*> sWndToWindow;
    static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

    WindowImplMsw::WindowImplMsw(Window* window, int32_t width, int32_t height)
      : WindowImpl(window, width, height)
    {
      const auto& appParmas = mWindow->getApp()->getAppParams();
      const auto& classNameUtf8 = appParmas->name;

      mWindowClassName.resize(classNameUtf8.size() + 1);
      size_t convertedChars = 0;
      mbstowcs_s(&convertedChars, mWindowClassName.data(), mWindowClassName.size(), classNameUtf8.c_str(), _TRUNCATE);

      WNDCLASSEX wc = {};
	    wc.cbSize         = sizeof(WNDCLASSEX);
	    wc.style          = CS_HREDRAW | CS_VREDRAW;
	    wc.lpfnWndProc    = WndProc;
	    wc.cbClsExtra     = NULL;
	    wc.cbWndExtra     = NULL;
	    wc.hInstance      = appParmas->hInstance;
	    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
	    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 2);
	    wc.lpszMenuName   = NULL;
	    wc.lpszClassName  = mWindowClassName.data();
	    wc.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

	    if (!RegisterClassEx(&wc)){
		    MessageBox(NULL, L"Error registering window class", L"Error", MB_OK | MB_ICONERROR);
        throw std::runtime_error("Error registering window class");
	    }

      DWORD dwStyle = WS_OVERLAPPEDWINDOW;
      LONG clientWidth = static_cast<LONG>(getWidth());
      LONG clientHeight = static_cast<LONG>(getHeight());
      RECT windowRect = {0, 0, clientWidth, clientHeight};
      AdjustWindowRect(&windowRect, dwStyle, FALSE);
      int windowWidth = windowRect.right - windowRect.left;
      int windowHeight = windowRect.bottom - windowRect.top;

	    mWnd = CreateWindowEx(NULL, mWindowClassName.data(), mWindowClassName.data(),
		    dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		    clientWidth, windowHeight, NULL, NULL, appParmas->hInstance, NULL);

	    if (! mWnd) {
		    MessageBox(NULL, L"Error creating window", L"Error", MB_OK | MB_ICONERROR);
		    throw std::runtime_error("Error creating window");
	    }

      mDc = GetDC(mWnd);
      sWndToWindow[mWnd] = mWindow;

      ShowWindow(mWnd, appParmas->nCmdShow);
      //UpdateWindow(mWnd);
    }

    WindowImplMsw::~WindowImplMsw() {
      ReleaseDC(mWnd, mDc);

      const auto& appParmas = mWindow->getApp()->getAppParams();
      UnregisterClass(mWindowClassName.data(), appParmas->hInstance);
    }

    void WindowImplMsw::redraw()
    {
	    ::RedrawWindow( mWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW );
    }

    void WindowImplMsw::setupRenderer() {
      mRenderer->setup(mWnd, mDc, nullptr);
    }

    static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
      Window* window = nullptr;
      if (sWndToWindow.find(wnd) != sWndToWindow.end()) {
        window = sWndToWindow[wnd];
      }
    
      switch (msg) {
        case WM_CREATE: {
          // NOTE: sWndWindow will not yet contain an entry for wnd since
          //       this message will be sent before CreateWindowEx finishes.
        } break;

        case WM_KEYDOWN: {
          assert(window != nullptr);
        } break; 

        case WM_PAINT: {
          assert(window != nullptr);
          window->private_draw();
        } break; 

        
        case WM_DESTROY: {
          PostQuitMessage(0);
          sWndToWindow.erase(wnd); 
          window->close();
          return 0;
        } break;
      }
      return DefWindowProc(wnd, msg, wParam, lParam);
    }
  } // namespace app
} // namespace cinder
#endif

// -------------------------------------------------------------------------------------------------
// Support functions
// -------------------------------------------------------------------------------------------------
namespace cinder {
  DataSourceRef loadFile(const fs::path& path) {
    std::ifstream is(path.c_str(), std::ios::binary);
    if (! is) {
      std::string msg = "Couldn't open " + path.str();
      throw std::runtime_error(msg);
    }

    is.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(is.tellg());
    is.seekg(0, std::ios::beg);

    BufferRef buffer = std::make_shared<Buffer>(size);
    if (! buffer) {
      throw std::runtime_error("Couldn't allocate buffer");
    }

    is.read(reinterpret_cast<char*>(buffer->getData()), size);

    DataSourceRef result = std::make_shared<DataSource>(buffer);
    return result;
  }

#if defined(LC_IMAGE_H)
  ImageSourceRef loadImage(const fs::path& path) {
    if (! fs::exists(path)) {
      std::string msg = "Couldn't find image file " + path.str();
      throw std::runtime_error(msg);
    }

    int width = 0;
    int height = 0;
    int channelCount = 0;
    unsigned char* pixels = lc_load_image(path.c_str(), &width, &height, &channelCount, 0); 
    if (pixels == nullptr) {
      std::string msg = "Couldn't load image " + path.str();
      throw std::runtime_error(msg);
    }

    int rowBytes = width*channelCount;
    size_t size = static_cast<size_t>(rowBytes * height);
    BufferRef buffer = std::make_shared<Buffer>(size);
    std::copy(pixels, pixels + size, buffer->getData());

    lc_free_image(pixels);

    ImageSourceRef result = std::make_shared<ImageSource>(width, height, 
                                                          channelCount, rowBytes, 
                                                          buffer);
    return result;
  }
#endif
} // namespace cinder

#endif // defined(TINYCI_IMPLEMENTATION)

#if defined(CINDER_MSW)
  #define CINDER_APP( APP, RENDERER, ... )                                                               \
    int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) \
    {                                                                                                    \
      auto appParams = new cinder::app::detail::AppParams();                                             \
      appParams->name = #APP;                                                                            \
      appParams->hInstance = hInstance;                                                                  \
      appParams->nCmdShow = nCmdShow;                                                                    \
      appParams->createRendererFn = []() -> cinder::app::Renderer* { return new RENDERER(); };           \
      auto settings = new App::Settings();                                                               \
      cinder::app::App::configureApp(appParams, settings, ##__VA_ARGS__);                                \
	    std::unique_ptr<ci::app::App> app = std::make_unique<APP>();                                       \
      app->private_run();                                                                                \
      return 0;                                                                                          \
    }
#elif defined(CINDER_LINUX)
#endif

#endif // TINYCI_H