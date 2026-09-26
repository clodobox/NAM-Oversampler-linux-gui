/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

// X11/Xlib.h and X11/X.h define macros that conflict with C++ identifiers in
// IGraphics and IPlug headers. We include X11 headers first to get the type
// declarations, then undef the conflicting macros, then include IGraphics headers.
// GL/glx.h must come AFTER IGraphics_select.h (which includes glad) because
// glad checks that GL/gl.h has not been included yet.
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Undef X11 macros that conflict with C++ enum identifiers and type names.
// IGraphicsLinux.cpp includes X11 headers directly before this header, so the
// macros are available there for X11 API calls. Function bodies use 0/1 literals
// instead of None/False/True to avoid relying on the undefined names.
#ifdef None
#undef None
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef Status
#undef Status
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Success
#undef Success
#endif
#ifdef Complex
#undef Complex
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif
#ifdef Convex
#undef Convex
#endif
#ifdef CurrentTime
#undef CurrentTime
#endif

// IGraphics_select.h includes <glad/glad.h> for Linux (defines __gl_h_).
// GL/glx.h must be included BEFORE this header (in the .cpp file) while Bool
// is still defined from Xlib.h; the GLX_H include guard prevents re-inclusion.
#include <atomic>
#include "IGraphics_select.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for Linux
*   @ingroup PlatformClasses */
class IGraphicsLinux final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsLinux();

  void ForceEndUserEdit() override;
  /** Returns the screen scale factor so that requestResize() and
   *  OnParentWindowResize() pass physical pixels, matching what the X11
   *  server reports via ConfigureNotify (ce.width / GetScreenScale()). */
  float GetPlatformWindowScale() const override { return GetScreenScale(); }

  /** Returns the X11 display file descriptor, or -1 if not open.
   *  Used by CLAP posix-fd support to register with the host's event loop. */
  int GetX11Fd() const { return mDisplay ? XConnectionNumber(mDisplay) : -1; }

  /** When host-driven = true the internal timer thread is suppressed.
   *  The host drives rendering via onTimer / onPosixFd callbacks instead.
   *  Safe to call after OpenWindow — stops the timer thread if running. */
  void SetHostDriven(bool hostDriven)
  {
    mHostDriven = hostDriven;
    if (hostDriven)
      StopTimer();
  }

  /** Called by CLAP host timer callback to advance one display frame. */
  void OnDisplayTimer();
  /** Called by CLAP host posix-fd callback to drain pending X11 events. */
  void ProcessX11Events();

  void PlatformResize(bool parentHasResized) override;
  void DrawResize() override;
  void OnBeginHostResize(int physW, int physH) override;
  void OnEndHostResize() override;

  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  ECursor SetMouseCursor(ECursor cursorType) override;

  void GetMouseLocation(float& x, float& y) const override;

  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler) override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return mPlugWnd != 0; }

  void UpdateTooltips() override {}

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override;
  void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  void* GetWindow() override { return (void*)(uintptr_t)mPlugWnd; }

  const char* GetPlatformAPIStr() override { return "x11"; }

  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override;

protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;

private:
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override;

  void ActivateGLContext() override;
  void DeactivateGLContext() override;

  XVisualInfo* CreateGLContext();  // returns the visual used; caller must XFree() it
  void DestroyGLContext();
  void* mGLContext  = nullptr;  // GLXContext
  Colormap mGLColormap = 0;

  Display* mDisplay = nullptr;
  Window mPlugWnd = 0;
  Window mParentWnd = 0;
  Atom mWMDeleteMessage = 0;
  std::atomic<bool> mTimerRunning{false};

  // Set when the window manager asks us to close (WM_DELETE_WINDOW). The close
  // itself must NOT happen inside ProcessX11Events() — that runs on the timer
  // thread and would destroy mDisplay/the IGraphics object out from under the
  // event pump (XPending(NULL) → SIGSEGV). OnDisplayTimer() acts on the flag
  // after the pump has returned instead.
  std::atomic<bool> mCloseRequested{false};

  // Cleared when the X server tells us mPlugWnd is gone (DestroyNotify). After
  // that, any further X/GL call on the drawable would raise an X error.
  std::atomic<bool> mWindowAlive{true};

  // glXMakeCurrent() is reference-counted here: IGraphicsNanoVG may activate the
  // context recursively (e.g. LoadAPIBitmap inside a frame), and the outermost
  // release is the only one that may unbind the context.
  int mGLContextDepth = 0;


  float mHiddenCursorX = 0.f;
  float mHiddenCursorY = 0.f;
  Cursor mBlankCursor = 0;  // cached invisible cursor for HideMouseCursor
  bool mNeedsRedraw = false;
  unsigned long mLastLeftClickTime = 0;  // X11 server timestamp of last left-button press

  // Clipboard atoms (interned in OpenWindow)
  Atom mClipboardAtom = 0;
  Atom mUTF8StringAtom = 0;
  Atom mTargetsAtom = 0;
  Atom mSelDataAtom = 0;
  WDL_String mClipboardText;  // text we own as selection provider

  bool mHostDriven = false;        // set true before OpenWindow when host drives the event loop
  bool mInConfigureNotify = false; // suppress PlatformResize re-entry from ConfigureNotify
  bool mInHostResize = false;      // suppress XResizeWindow during host-driven resize
  bool mHostDidResize = false;     // set by OnEndHostResize; cleared by PlatformResize

  // The physical size the host last passed via onSize → OnParentWindowResize.
  // OnEndHostResize uses this to XResizeWindow to exactly the host's size,
  // avoiding aspect-ratio drift from Scale mode's max(scaleX, scaleY).
  unsigned mHostPhysW = 0;
  unsigned mHostPhysH = 0;

  // Last physical size we actually passed to XResizeWindow (or XCreateWindow).
  // ConfigureNotify events for any other size are stale (from a superseded
  // XResizeWindow request) and must be ignored to prevent mWidth from reverting.
  unsigned mLastPhysW = 0;
  unsigned mLastPhysH = 0;

  // ---- Out-of-process dialogs ----------------------------------------------
  // An in-process GTK dialog runs a nested GTK main loop (gtk_dialog_run), and
  // GTK only supports one main loop, on the thread that initialised it. When the
  // host is itself GTK-based (REAPER's SWELL, Bitwig, ...), running our own
  // gtk_main() from the render thread races the host's GTK state, wedges the
  // GfxMutex that the dialog is holding, and freezes the host — the same failure
  // that forced popup menus onto the self-drawn path. Where `zenity` exists we
  // run the dialog in a child process instead and deliver the result back to the
  // render thread.
  struct DialogResult
  {
    enum class Kind
    {
      File,
      Directory,
      MessageBox
    } kind = Kind::File;

    WDL_String file;
    WDL_String path;
    EMsgBoxResult msgResult = EMsgBoxResult::kOK;
    IFileDialogCompletionHandlerFunc fileHandler;
    IMsgBoxCompletionHandlerFunc msgHandler;
  };

  // Shared with a detached helper thread. The helper only ever touches the
  // slot, never this object, so a dialog still running when the window closes
  // cannot become a use-after-free — CloseWindow() marks the slot abandoned and
  // the helper's write is simply dropped.
  struct DialogSlot
  {
    std::mutex mutex;
    bool abandoned = false;
    bool hasResult = false;
    DialogResult result;
  };

  std::vector<std::shared_ptr<DialogSlot>> mDialogSlots;  // render thread only
  void DrainPendingDialogs();
  void StartOutOfProcessFileDialog(bool isDirectory, bool isSave,
                                   const WDL_String& initialPath,
                                   const WDL_String& initialFileName,
                                   const char* ext,
                                   IFileDialogCompletionHandlerFunc completionHandler);

  void StartTimer();
  void StopTimer();

  static void* TimerThreadProc(void* pParam);
  pthread_t mTimerThread = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
