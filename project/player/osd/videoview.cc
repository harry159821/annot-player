// videoview.cc
// 7/10/2011

#include "videoview.h"
#include "global.h"
#ifdef USE_MAC_VLCKIT
  #include "mac/vlckit_qt/vlckit_qt.h"
  #include "mac/qtstep/qtstep.h"
#endif // USE_MAC_VLCKIT
#ifdef USE_WIN_HOOK
  #include "win/hook/hook.h"
  #include "win/qtwin/qtwin.h"
#endif // USE_WIN_HOOK
#include <QtGui>
#ifdef Q_WS_X11
  //#include <QX11Info>
  #include <X11/Xlib.h>
#endif // Q_WS_X11

#ifdef Q_WS_MAC
  #define BASE(_parent)   Base(0, _parent)
#else
  #define BASE(_parent)   Base(_parent)
#endif // Q_WS_MAC

// - Constructions -
VideoView::VideoView(QWidget *parent)
  : BASE(parent)
{
  setContentsMargins(0, 0, 0, 0);
  //setAttribute(Qt::WA_TransparentForMouseEvents);
  //setWindowOpacity(1.0); // opaque
  //setMouseTracking(true);

#ifdef USE_MAC_VLCKIT
  view_ = ::vlcvideoview_new();
  //setCocoaView(view_);
#endif // USE_MAC_VLCKIT

#ifdef Q_WS_X11
  //connect(this, SIGNAL(clientIsEmbedded()), SLOT(invalidateClientWindow()));
  //connect(this, SIGNAL(clientClosed()), SLOT(invalidateClientWindow()));
#endif // Q_WS_X11
}
#undef BASE

VideoView::~VideoView()
{
#ifdef USE_MAC_VLCKIT
  if (view_)
    ::vlcvideoview_release((vlcvideoview_t*)view_);
#endif // USE_MAC_VLCKIT
}

// - Mac OS X Cocoa View -

#ifdef Q_WS_MAC

void*
VideoView::view() const
{ return view_; }

bool
VideoView::isViewVisible() const
{
  //nsview_t *view = reinterpret_cast<nsview_t*>(cocoaView());
  //return view && !::nsview_is_hidden(view);
  return cocoaView();
}

void
VideoView::setViewVisible(bool visible)
{
  //nsview_t *view = reinterpret_cast<nsview_t*>(cocoaView());
  //if (view)
  //  ::nsview_set_hidden(view, !visible);
  if (visible) {
    if (!cocoaView())
      setCocoaView(view_);
  } else {
    if (cocoaView())
      setCocoaView(0);
  }
}

void
VideoView::showView()
{ setViewVisible(true); }

void
VideoView::hideView()
{ setViewVisible(false); }

#endif // Q_WS_MAC

// - Windows Hook -

#ifdef USE_WIN_HOOK

//#define DEBUG "VideoView"
#include "module/debug/debug.h"

// This function is re-entrant, and \var tries is not synchornized
void
VideoView::addToWindowsHook()
{
  enum { VLC_CHILDREN_COUNT = 2 };
  enum { VLC_PLAYING_TIMEOUT = 200 }; // in msecs
  enum { MAX_TRIES = 50 }; // in total 10 secs
  static int tries = 0; // retries count

  DOUT("addToWindowsHook:enter: tries =" << tries);
  //if (children_.size() >= VLC_CHILDREN_COUNT) {
  if (tries > MAX_TRIES || children_.size() >= 2 * VLC_CHILDREN_COUNT) {
    // This should never happen.
    //Q_ASSERT(0);
    DOUT("addToWindowsHook:exit: max tries/children already reached");
    tries = 0;
    return;
  }

  WId hwnd = winId();
  while (hwnd = QtWin::getChildWindow(hwnd))
    if (children_.indexOf(hwnd) < 0) { // if not existed
      children_.append(hwnd);
      HOOK->addWinId(hwnd);
      DOUT("add hwnd to hook:" << hwnd << ", tries =" << tries);
    }

  // jichi 8/10/2011: VLC player usually added two child windows.

  //if (children_.size() < VLC_CHILDREN_COUNT) {
  //DOUT("not hook enough child windows, try again soon: tries =" << tries);
  if (tries < MAX_TRIES &&
      children_.size() < 2 * VLC_CHILDREN_COUNT)  {
    tries++;
    QTimer::singleShot(VLC_PLAYING_TIMEOUT, this, SLOT(addToWindowsHook()));
  } else {
    tries = 0;
    DOUT("max number of tries/children reached, stop watching child windows");
  }

  DOUT("addToWindowsHook:exit");
}

void
VideoView::removeFromWindowsHook()
{
  DOUT("removeFromWindowsHook:enter");
  if (!children_.empty()) {
    foreach (WId hwnd, children_)
      HOOK->removeWinId(hwnd);
    children_.clear();
    DOUT("hooked hwnd removed");
  }
  DOUT("removeFromWindowsHook:exit");
}

bool
VideoView::containsWindow(WId hwnd) const
{ return children_.contains(hwnd); }

#endif // USE_WIN_HOOK

// - X11 Events -

/*
// See: http://www.qtcentre.org/threads/33561-Handling-mouse-events-in-Qt-WebKit
void
VideoView::invalidateClientWindow()
{
  WId cid = clientWinId();
  if (!cid)
    return;
  int ok = ::XGrabButton(
    QX11Info::display(),
    AnyButton,      // any mouse button
    AnyModifier,    // any keyboard modifier
    cid,
    False,          // owner events ?
    ButtonPressMask,
    GrabModeAsync,  // pointer mode
    GrabModeAsync,  // keyboard mode
    0,  // window to confine to
    0   // cursor
  );
}
*/

#ifdef Q_WS_X11
// CHECKPOINT: simulate double click event here

bool
VideoView::x11Event(XEvent *event)
{
  // Note: event is ALWAYS forwarded to base class, and then to clientWindow (VLC)

  //enum { DOUBLE_CLICK_TIMEOUT = 500 }; // time between D and DUD, in msecs
  //static Time recentClickTime_ = 0; // in msecs

  Q_ASSERT(event);
  switch (event->type) {
  case ButtonPress:
  case ButtonRelease:
    if (clientWinId()) {
      XEvent e = *event;
      e.xbutton.window = clientWinId();
      // XSendEvent is blocking send, therefore e could be on the stack.
      // propagage = True, event_mask = 0;
      ::XSendEvent(event->xbutton.display, clientWinId(), True, 0, &e);
    }
  case MotionNotify:
    if (parentWidget()) {
      /*
      Window root, child;
      Display *dpy = event->xbutton.display;
      int root_x, root_y, win_x, win_y;
      uint mask = 0;

      Window target;
      if (::XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child,
                          &root_x, &root_y, &win_x, &win_y, &mask))
        target = child ? child : root;
      else
        target = parentWidget()->winId();
      */
      Window window = parentWidget()->winId();
      XEvent e = *event;
      e.xbutton.window = window;
      ::XSendEvent(event->xbutton.display, window, True, 0, &e); // propagate = True, event_mask = 0
    } break;

    /*
    if (parentWidget()) {
      QPoint pos(event->xbutton.x, event->xbutton.y),
             globalPos(event->xbutton.x_root, event->xbutton.y_root);

      QMouseEvent::Type type = event->type == ButtonPress ? QMouseEvent::MouseButtonPress
                                                          : QMouseEvent::MouseButtonRelease;

      Qt::MouseButton button;
      switch (event->xbutton.button) {
      case Button3: button = Qt::RightButton; recentClickTime_ = 0; break;
      case Button2: button = Qt::MiddleButton; recentClickTime_ = 0; break;

      case Button1: button = Qt::LeftButton;
        if (event->type == ButtonPress) {
          if (recentClickTime_) {
            if (event->xbutton.time - recentClickTime_< DOUBLE_CLICK_TIMEOUT)
              type = QMouseEvent::MouseButtonDblClick;
            recentClickTime_ = 0;
          } else
            recentClickTime_ = event->xbutton.time;
        }break;

      default: button = Qt::NoButton; recentClickTime_ = 0; break;
      }
      Qt::MouseButtons buttons = button;

      QCoreApplication::postEvent(parentWidget()
        new QMouseEvent(type, pos, globalPos, button, buttons, Qt::NoModifier)
      );
      //return true;
    } break;

  case MotionNotify:
    recentClickTime_ = 0;
    if (parentWidget()) {
        QPoint pos(event->xmotion.x, event->xbutton.y),
               globalPos(event->xmotion.x_root, event->xbutton.y_root);

      // jichi 11/25/2011: No buttons specified since using XQueryPointer to query buttons would be expensive.

      QCoreApplication::postEvent(parentWidget(),
        new QMouseEvent(QMouseEvent::MouseMove, pos, globalPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier)
      );
      //return true;
    } break;
    */

  default: break;
  }
  return Base::x11Event(event);
}

#endif // Q_WS_X11

// EOF

/*

void
VideoView::mouseMoveEvent(QMouseEvent *event)
{
  if (event && event->buttons() & Qt::LeftButton) {
    QPoint newPos = event->globalPos() - dragPos_;
    move(newPos);
    event->accept();
  }
}

void
VideoView::mouseReleaseEvent(QMouseEvent *event)
{ Q_UNUSED(event); }

void
VideoView::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (event)
    event->accept();
}
*/

