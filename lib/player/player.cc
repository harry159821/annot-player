// player.cc
// 6/30/2011

#ifdef _MSC_VER
# pragma warning (disable:4819)       // C4819: The file contains a character that cannot be represented in the current code page.
# pragma warning (disable:4996)       // C4996: MS' deprecated std functions orz.
#endif // _MSC_VER

#ifndef __LIBVLC__
# define __LIBVLC__ // disable unnecessary warnings
#endif // __LIBVLC__

#include "lib/player/player.h"
#include "lib/player/playerdefs.h"
#include "lib/player/player_p.h"
//#include "qtx/qxos.h"
#ifdef WITH_LIB_VLCCORE
# include "lib/vlccore/video.h"
# include "lib/vlccore/sound.h"
# ifdef Q_OS_WIN
#  ifdef WITH_WIN_QTWIN
#   include "win/qtwin/qtwin.h"
#  else
#   warning "qtwin is not used"
#  endif // WITH_WIN_QTWIN
# endif // Q_OS_WIN
#endif // WITH_LIB_VLCCORE
#include <QtCore/QEventLoop>
#ifdef Q_OS_MAC
# include <QtGui/QMacCocoaViewContainer>
#endif // Q_OS_MAC
#include <QtGui/QMouseEvent>
#include <QtNetwork/QNetworkCookieJar>
#include <boost/tuple/tuple.hpp>
#include <boost/typeof/typeof.hpp>
#include <cstring>
#include <memory>

#ifndef MODULE_STRING
# define MODULE_STRING "main"  // needed by VLC
#endif
#include <inttypes.h>
#include <vlc/plugins/vlc_vout.h>
#include <vlc/plugins/vlc_input_item.h>
#include <vlc/lib/libvlc_internal.h>
#include <vlc/lib/media_internal.h>
#include <vlc/lib/media_list_internal.h>
#include <vlc/vlc.h>
//#include <vlc/lib/media_list.c>

//#define DEBUG "player"
#include "qtx/qxdebug.h"

// - Settings -

enum { VOUT_COUNTDOWN = 30 }; // vout countdown timer
enum { VOUT_TIMEOUT = 500 };  // 0.5 secs

// - PlayerPrivate -

class PlayerPrivate
  : public detail::mp_handle,
    public detail::mp_states,
    public detail::mp_properties,
    public detail::mp_trackers
    //public detail::mp_intl
{ };

// - Event handlers -
// See libvlc_event.h.
//
// libvlc_MediaMetaChanged
// libvlc_MediaSubItemAdded
// libvlc_MediaDurationChanged
// libvlc_MediaParsedChanged
// libvlc_MediaFreed
// libvlc_MediaStateChanged
// libvlc_MediaPlayerMediaChanged
// libvlc_MediaPlayerNothingSpecial
// libvlc_MediaPlayerOpening
// libvlc_MediaPlayerBuffering
// libvlc_MediaPlayerPlaying
// libvlc_MediaPlayerPaused
// libvlc_MediaPlayerStopped
// libvlc_MediaPlayerForward
// libvlc_MediaPlayerBackward
// libvlc_MediaPlayerEndReached
// libvlc_MediaPlayerEncounteredError
// libvlc_MediaPlayerTimeChanged
// libvlc_MediaPlayerPositionChanged
// libvlc_MediaPlayerSeekableChanged
// libvlc_MediaPlayerPausableChanged
// libvlc_MediaPlayerTitleChanged
// libvlc_MediaPlayerSnapshotTaken
// libvlc_MediaPlayerLengthChanged
// libvlc_MediaListItemAdded
// libvlc_MediaListWillAddItem
// libvlc_MediaListItemDeleted
// libvlc_MediaListWillDeleteItem
// libvlc_MediaListViewItemAdded
// libvlc_MediaListViewWillAddItem
// libvlc_MediaListViewItemDeleted
// libvlc_MediaListViewWillDeleteItem
// libvlc_MediaListPlayerPlayed
// libvlc_MediaListPlayerNextItemSet
// libvlc_MediaListPlayerStopped
// libvlc_MediaDiscovererStarted
// libvlc_MediaDiscovererEnded
// libvlc_VlmMediaAdded
// libvlc_VlmMediaRemoved
// libvlc_VlmMediaChanged
// libvlc_VlmMediaInstanceStarted
// libvlc_VlmMediaInstanceStopped
// libvlc_VlmMediaInstanceStatusInit
// libvlc_VlmMediaInstanceStatusOpening
// libvlc_VlmMediaInstanceStatusPlaying
// libvlc_VlmMediaInstanceStatusPause
// libvlc_VlmMediaInstanceStatusEnd
// libvlc_VlmMediaInstanceStatusError
//
// typedef void(* libvlc_callback_t)(const struct libvlc_event_t *, void *);
namespace { // anonymous, vlc event callbacks
  namespace vlc_event_handler_ {
#define VLC_EVENT_HANDLER(_signal) \
    void \
    _signal(const struct libvlc_event_t *event, void *media_player_instance) \
    { \
      DOUT("enter"); \
      Q_UNUSED(event) \
      Player *p = static_cast<Player *>(media_player_instance); \
      Q_ASSERT(p); \
      p->emit_##_signal(); \
      DOUT("exit"); \
    }

    VLC_EVENT_HANDLER(opening)
    VLC_EVENT_HANDLER(buffering)
    VLC_EVENT_HANDLER(playing)
    VLC_EVENT_HANDLER(stopped)
    VLC_EVENT_HANDLER(paused)
    VLC_EVENT_HANDLER(timeChanged)
    //VLC_EVENT_HANDLER(lengthChanged)
    VLC_EVENT_HANDLER(positionChanged)
    VLC_EVENT_HANDLER(mediaChanged)
    //VLC_EVENT_HANDLER(errorEncountered)
    VLC_EVENT_HANDLER(endReached)
#undef MEDIA_PLAYER_EVENT_HANDLER

    void
    lengthChanged(const struct libvlc_event_t *event, void *media_player_instance)
    {
      DOUT("enter");
      Q_UNUSED(event)
      Player *p = static_cast<Player *>(media_player_instance);
      Q_ASSERT(p);
      p->emit_lengthChanged();

      // Addtional operations
      p->updateTitleId();

      DOUT("exit");
    }

    void
    errorEncountered(const struct libvlc_event_t *event, void *media_player_instance)
    {
      DOUT("enter");
      Q_UNUSED(event)
      Player *p = static_cast<Player *>(media_player_instance);
      Q_ASSERT(p);
      //p->emit_errorEncountered();
      p->handleError();
      DOUT("exit");
    }
  }
} // anonymous namespace


#ifdef WITH_LIB_VLCCORE
namespace { // anonymous, vlccore callbacks

  namespace vout_callback_ { // Consisent with vlc_callback_t

    int doubleClickTimeout_ = // time between D and DUD, in msecs
#ifdef WITH_WIN_QTWIN
     QtWin::getDoubleClickInterval() * 1.5
#else
     800
#endif // Q_OS_WIN
    ;

    qint64 recentClickTime_ = 0; // in msecs
    bool ignoreNextMove_ = false;

    // Not used
    //int
    //mouse_clicked(vlc_object_t *p_this, char const *psz_var,
    //              vlc_value_t oldval, vlc_value_t newval, void *p_data)
    //{
    //  qDebug() << "mouse-clicked";
    //  return 0;
    //}

    int
    mouse_moved(vlc_object_t *p_this, char const *psz_var,
                vlc_value_t oldval, vlc_value_t newval, void *p_data )
    {
      //qDebug() << "mouse-moved";
      Q_UNUSED(psz_var) // Q_ASSERT(psz_var == "mouse-moved");
      Q_UNUSED(oldval)

      if (ignoreNextMove_) {
        ignoreNextMove_ = false;
        return VLC_SUCCESS;
      }
      recentClickTime_ = 0;

      Player *player = reinterpret_cast<Player *>(p_data);
      Q_ASSERT(player);
      if (!player || !player->isMouseEventEnabled())
        return 0;
      vout_thread_t *vout = reinterpret_cast<vout_thread_t *>(p_this);
      Q_ASSERT(vout);
      //if (!vout ||
      //    !vout->render.i_width || !vout->render.i_height)
      //  return 0;
      if (!vout)
        return 0;
      QWidget *w = player->d()->voutWindow();
      if (!w)
        return 0;

      vlccore::vlcbuttons bt = vlccore::vout_mouse_buttons(vout);

      QPoint pos(newval.coords.x, newval.coords.y); // same as libvlc_video_get_cursor
      //QPoint pos = vlccore::vout_map_to_widget(vout, coords, w->size());
      QPoint globalPos =
#ifdef WITH_WIN_QTWIN
          QtWin::getMousePos()
#else
          pos + w->mapToGlobal(QPoint())
#endif // Q_OS_WIN
      ;

      Qt::MouseButton button;
      Qt::MouseButtons buttons;
      boost::tie(button, buttons) = vlccore::vlcbuttons_to_qt(bt);

//#ifdef Q_OS_WIN
//      // Mouse-move without buttons is already provided by mousehook on Windows
//      // Skip to reduce overheads.
//      if (!buttons)
//        return VLC_SUCCESS;
//#endif // Q_OS_WIN

      // Post event across diff threads.
      QCoreApplication::postEvent(w,
        new QMouseEvent(QEvent::MouseMove, pos, globalPos, button, buttons, Qt::NoModifier)
      );

      return VLC_SUCCESS;
    }

    int
    mouse_button_down(vlc_object_t *p_this, char const *psz_var,
                      vlc_value_t oldval, vlc_value_t newval, void *p_data)
    {
      //qDebug() << "mouse-button-down" << newval.i_int;
      Q_UNUSED(psz_var) // Q_ASSERT(psz_var == "mouse-button-down");

      qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

      Player *player = reinterpret_cast<Player *>(p_data);
      Q_ASSERT(player);
      if (!player || !player->isMouseEventEnabled())
        return 0;
      vout_thread_t *vout = reinterpret_cast<vout_thread_t *>(p_this);
      Q_ASSERT(vout);
      if (!vout)
        return 0;
      QWidget *w = player->d()->voutWindow();
      if (!w)
        return 0;

      QPoint pos = vlccore::vout_mouse_pos(vout); // same as libvlc_video_get_cursor
      //QPoint pos = vlccore::vout_map_to_widget(vout, coords, w->size());
      QPoint globalPos =
#ifdef WITH_WIN_QTWIN
          QtWin::getMousePos()
#else
          pos + w->mapToGlobal(QPoint())
#endif // Q_OS_WIN
      ;

      Qt::MouseButton button;
      Qt::MouseButtons buttons;
      boost::tie(button, buttons) = vlccore::vlcbuttons_to_qt(newval.i_int);

      QEvent::Type type = button ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease;
      if (!button)
        boost::tie(button, buttons) = vlccore::vlcbuttons_to_qt(oldval.i_int);

      if (button != Qt::LeftButton)
        recentClickTime_ = 0;
      else if (type == QEvent::MouseButtonPress) {
        if (recentClickTime_) {
          if (currentTime - recentClickTime_ < doubleClickTimeout_)
            type = QMouseEvent::MouseButtonDblClick;
          recentClickTime_ = 0;
        } else
          recentClickTime_ = currentTime;
      }

#ifdef WITH_WIN_QTWIN
      if (type != QMouseEvent::MouseButtonRelease && button == Qt::LeftButton) {
        // Disable VLC built-in double-click timer.
        ignoreNextMove_ = true;

        static bool rand_;
        rand_ = !rand_;
        int offset = rand_ ? 4 : -4;
        QtWin::sendMouseMove(QPoint(offset, 0), true); // relative == true
      }
#endif // Q_OS_WIN

      // Post event across diff threads.
      QCoreApplication::postEvent(w,
        new QMouseEvent(type, pos, globalPos, button, buttons, Qt::NoModifier)
      );

      return VLC_SUCCESS;
    }

  } // namespace callback_

  ///  Return number of new vouts.
  int
  register_vout_callbacks_(libvlc_media_player_t *mp, Player *player)
  {
    static QList<vout_thread_t *> vouts_;

    Q_ASSERT(mp);
    vout_thread_t **vouts;
    size_t vouts_count;

    vouts = vlccore::GetVouts(mp, &vouts_count);
    if (vouts)
      for (size_t i = 0; i < vouts_count; i++) {
        vout_thread_t *vout = vouts[i];
        Q_ASSERT(vout);
        if (vout && !vouts_.contains(vout)) {
          vouts_.append(vout);
          ::var_AddCallback(vout, "mouse-moved", vout_callback_::mouse_moved, player);
          ::var_AddCallback(vout, "mouse-button-down", vout_callback_::mouse_button_down, player);
          //::var_AddCallback(vout, "mouse-clicked", vout_callback_::mouse_clicked, player);
        }
      }
    return vouts_count;
  }

} // anonymous namespace
#endif // WITH_LIB_VLCCORE

// - Static properties -

const QStringList&
Player::supportedSuffices()
{
  static const QStringList ret =
    supportedVideoSuffices() +
    supportedAudioSuffices() +
    supportedPictureSuffices() +
    supportedSubtitleSuffices() +
    supportedPlaylistSuffices() +
    supportedImageSuffices();
  return ret;
}

const QStringList&
Player::supportedFilters()
{
  static const QStringList ret =
    supportedVideoFilters() +
    supportedAudioFilters() +
    supportedPictureFilters() +
    supportedSubtitleFilters() +
    supportedPlaylistFilters() +
    supportedImageFilters();
  return ret;
}

bool
Player::isSupportedFile(const QString &fileName)
{
  foreach (const QString &suffix, supportedSuffices())
    if (fileName.endsWith("." + suffix, Qt::CaseInsensitive))
      return true;
  return false;
}

#define SUPPORTED_SUFFICES_(_Type, _TYPE) \
  const QStringList& \
  Player::supported##_Type##Suffices() \
  { \
    static const QStringList ret = \
      QStringList() PLAYER_FORMAT_##_TYPE(<<); \
    return ret; \
  }

  SUPPORTED_SUFFICES_(Video, VIDEO)
  SUPPORTED_SUFFICES_(Audio, AUDIO)
  SUPPORTED_SUFFICES_(Picture, PICTURE)
  SUPPORTED_SUFFICES_(Subtitle, SUBTITLE)
  SUPPORTED_SUFFICES_(Playlist, PLAYLIST)
  SUPPORTED_SUFFICES_(Image, IMAGE)
#undef SUPPORTED_SUFFICES_

#define SUPPORTED_FILTERS_(_Type, _TYPE) \
  const QStringList& \
  Player::supported##_Type##Filters() \
  { \
    static const QStringList ret = \
      QStringList() PLAYER_FORMAT_##_TYPE(<< "*."); \
    return ret; \
  }

  SUPPORTED_FILTERS_(Video, VIDEO)
  SUPPORTED_FILTERS_(Audio, AUDIO)
  SUPPORTED_FILTERS_(Picture, PICTURE)
  SUPPORTED_FILTERS_(Subtitle, SUBTITLE)
  SUPPORTED_FILTERS_(Playlist, PLAYLIST)
  SUPPORTED_FILTERS_(Image, IMAGE)
#undef SUPPORTED_FILTERS_

#define IS_SUPPORTED_(_type) \
  bool \
  Player::isSupported##_type(const QString &fileName) \
  { \
    foreach (const QString &suffix, supported##_type##Suffices()) \
      if (fileName.endsWith("." + suffix, Qt::CaseInsensitive)) \
        return true; \
    return false; \
  }

  IS_SUPPORTED_(Video)
  IS_SUPPORTED_(Audio)
  IS_SUPPORTED_(Picture)
  IS_SUPPORTED_(Subtitle)
  IS_SUPPORTED_(Playlist)
  IS_SUPPORTED_(Image)
#undef IS_SUPPORTED_

// - Constructions -

Player::Player(QObject *parent)
  : Base(parent), d_(nullptr)
{
  DOUT("enter");
#ifdef WITH_LIB_VLCHTTP
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(message(QString)), SIGNAL(message(QString)));
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(errorMessage(QString)), SIGNAL(errorMessage(QString)));
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(warning(QString)), SIGNAL(warning(QString)));
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(fileSaved(QString)), SIGNAL(fileSaved(QString)));
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(progress(qint64,qint64)), SIGNAL(downloadProgress(qint64,qint64)));
  connect(VlcHttpPlugin::globalInstance(), SIGNAL(buffering()), SIGNAL(buffering()), Qt::QueuedConnection);
#endif // WITH_LIB_VLCHTTP
  DOUT("exit");
}

Player::~Player()
{
  DOUT("enter");
  destroy();
  DOUT("exit");
}

bool
Player::isValid() const
{ return d_ && d_->valid(); }

void
Player::destroy()
{
  if (d_) {
    //if (d_->codec())
    //  delete d_->codec();
    delete d_;
  }
}

void
Player::reset()
{
  DOUT("enter");

  destroy();

  d_ = new Private;
  d_->reset();
  Q_ASSERT(isValid());

  enum { initial_volume = int(0.5 * VLC_MAX_VOLUME) };
  ::libvlc_audio_set_volume(d_->player(), initial_volume);

  setUserAgent();

  attachEvents();

  // Set timer.
  //d_->setUpdateTimer(new QTimer(this));
  //connect(d_->updateTimer(), SIGNAL(timeout()), SLOT(update()));
  //setUpdateInterval();

  DOUT("exit");
}

#define REGISTER_EVENTS(_attach) \
  void \
  Player::_attach##Events() \
  { \
    libvlc_event_manager_t *event_manager = ::libvlc_media_player_event_manager(d_->player()); \
    Q_ASSERT(event_manager); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerOpening, vlc_event_handler_::opening, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerBuffering, vlc_event_handler_::buffering, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerPlaying, vlc_event_handler_::playing, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerPaused, vlc_event_handler_::paused, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerStopped, vlc_event_handler_::stopped, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerTimeChanged, vlc_event_handler_::timeChanged, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerLengthChanged, vlc_event_handler_::lengthChanged, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerPositionChanged, vlc_event_handler_::positionChanged, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerMediaChanged, vlc_event_handler_::mediaChanged, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerEncounteredError, vlc_event_handler_::errorEncountered, this); \
    ::libvlc_event_##_attach(event_manager, libvlc_MediaPlayerEndReached, vlc_event_handler_::endReached, this); \
  }

  REGISTER_EVENTS(attach)
  REGISTER_EVENTS(detach)
#undef REGISTER_EVENTS

// - Embedding -

void
Player::setEmbeddedWindow(WindowHandle handle)
{
  Q_ASSERT(isValid());
  Q_ASSERT(handle);
  if (handle) {
    d_->setEmbedded();
    ::libvlc_media_player_set_drawable(d_->player(), handle);
  }
}

Player::WindowHandle
Player::embeddedWindow() const
{
  Q_ASSERT(isValid());
  return static_cast<WindowHandle>(
    ::libvlc_media_player_get_drawable(d_->player())
  );
}

#ifdef Q_OS_MAC
void
Player::embed(QMacCocoaViewContainer *w)
{
  DOUT("enter");
  Q_ASSERT(isValid());
  Q_ASSERT(w);
  if (w)
    setEmbeddedWindow(w->cocoaView());
  d_->setVoutWindow(w);
  DOUT("exit");
}
#else
void
Player::embed(QWidget *w)
{
  DOUT("enter");
  Q_ASSERT(isValid());
  Q_ASSERT(w);
  if (w)
    setEmbeddedWindow(w->winId());

  d_->setVoutWindow(w);
  DOUT("exit");
}
#endif // Q_OS_MAC

bool
Player::isEmbedded() const
{
  Q_ASSERT (isValid());
  return d_->isEmbedded();
}

// - Encoding -
//QString
//Player::encoding() const
//{
//  Q_ASSERT(isValid());
//  return d_->codec()->encoding();
//}
//
//void
//Player::setEncoding(const QString &encoding)
//{
//  Q_ASSERT(isValid());
//
//  if (d_->codec())
//    delete d_->codec();
//  d_->setCodec(new Core::TextCodec(encoding));
//  emit encodingChanged();
//}

//QByteArray
//Player::decode(const QString &input) const
//{
//  //return d_ && d_->codec() ?
//  //  d_->codec()->encode(input) :
//  return input.toAscii();
//}

//QString
//Player::encode(const char *input) const
//{
//  //return d_ && d_->codec() ?
//  //  d_->codec()->decode(input) :
//  return QString(input);
//}

// - Playing states -
bool
Player::isPlaying() const
{ return isValid() && ::libvlc_media_player_is_playing(d_->player()); }

bool
Player::isPaused() const
{ return isValid() && d_->isPaused(); }

bool
Player::isStopped() const
{ return !isPlaying() && !isPaused(); }

Player::Status
Player::status() const
{
  return !isValid() ? Stopped :
         isPlaying() ? Playing :
         isPaused() ? Paused :
         Stopped;
}

bool
Player::pausable() const
{ return hasMedia() && ::libvlc_media_player_can_pause(d_->player()); }

bool
Player::seekable() const
{ return hasMedia() && ::libvlc_media_player_is_seekable(d_->player()); }

// - Open media -

void
Player::clearMedia()
{
  DOUT("enter");
  Q_ASSERT(isValid());
  if (hasMedia())
    closeMedia();
  DOUT("exit");
}

bool
Player::openMediaAsCD(const QString &path)
{
  QString mrl = path;
  if (!mrl.startsWith(PLAYER_URL_CD, Qt::CaseInsensitive))
    mrl = PLAYER_URL_CD + mrl;
  if (mrl.endsWith('/'))
    mrl[mrl.length() - 1] = '\\';
  return openMedia(mrl);
}

void
Player::setStream(const QStringList &mrls, const QString &url, qint64 duration)
{
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::setUrls(mrls);
  VlcHttpPlugin::setOriginalUrl(url);
  VlcHttpPlugin::setDuration(duration);
#else
  Q_UNUSED(mrls)
  Q_UNUSED(url)
  Q_UNUSED(duration)
#endif // WITH_LIB_VLCHTTP
}

//bool
//Player::openStream(const QStringList &mrls)
//{
//  if (hasMedia())
//    closeMedia();
//  if (mrls.isEmpty())
//    return false;
//  setStream(mrls);
//  return openMedia(mrls.first());
//}

bool
Player::openMedia(const QString &path)
{
  DOUT("enter: path =" << path);
  Q_ASSERT(isValid());
  if (hasMedia())
    closeMedia();

  DOUT("open:" << path);

  // Handle CDA
  if (path.endsWith(".cda", Qt::CaseInsensitive)) {
    QFileInfo fi(path);
    QRegExp rx("^Track(\\d+)\\.cda$", Qt::CaseInsensitive);
    if (rx.indexIn(fi.fileName()) >= 0) {
      bool ok;
      int track1 = rx.cap(1).toInt(&ok);
      if (ok && track1 >= 1) {
        d_->setTrackNumber(track1 - 1);
        bool ret = openMediaAsCD(fi.absolutePath());
        DOUT("exit from cda code path: ret =" << ret);
        return ret;
      }
    }
  }

  if (isSupportedPlaylist(path)) {
#ifdef USE_PLAYER_PLAYLIST
    QList<libvlc_media_t *> l = parsePlaylist(path);
    if (l.isEmpty()) {
      DOUT("empty play list");
      emit errorEncountered();
      DOUT("exit: ret = false");
      return false;
    }

    d_->setMediaList(l);
    setTrackNumber(d_->trackNumber());
    DOUT("exit: ret = true");
    return true;
#else
    DOUT("exit: playlist support is not enabled");
    return false;
#endif // USE_PLAYER_PLAYLIST
  }

  libvlc_media_t *md = path.contains("://") ?
    ::libvlc_media_new_location(d_->instance(), _cs(path)) :   // MRL
    ::libvlc_media_new_path(d_->instance(), detail::vlcpath(path)); // local file
  d_->setMedia(md);

  //libvlc_media_list_player_t *mlp = libvlc_media_list_player_new(d_->instance());
  //libvlc_media_list_player_set_media_player(mlp, d_->player());
  //libvlc_media_list_t *ml = ::libvlc_media_list_new(d_->instance());
  //libvlc_media_list_add_media(ml, d_->media());
  //libvlc_media_list_add_file_content(ml, "d:/dev/mp4.mp4");
  //libvlc_media_list_add_file_content(ml, "d:/dev/mp4.mp4");
  //libvlc_media_list_player_set_media_list(mlp, ml);

  if (!d_->media()) {
    DOUT("WARNING: failed");
    DOUT("exit: ret = false");
    return false;
  }

  d_->setMediaPath(path);

  QFileInfo fi(path);
  if (!fi.exists())
    fi = QFileInfo(QUrl(path).toLocalFile());
  if (fi.exists())
    d_->setMediaSize(fi.size());

  ::libvlc_media_player_set_media(d_->player(), d_->media());

  if (isMouseEventEnabled())
    startVoutTimer();

  DOUT("exit: ret = true");
  return true;
}

void
Player::closeMedia()
{
  DOUT("enter");
  enum { ProcessEventsTimeout = 2000 }; // 2 seconds

  Q_ASSERT(isValid());
  if (isMouseEventEnabled())
    stopVoutTimer();
  if (!isStopped()) {
    stop();

    //enum { timeout = 1000 };
    //qDebug() << "Player::closeMedia: enter sleep: timeout =" << timeout;
    //Qxsleep(timeout);
    //qDebug() << "Player::closeMedia: leave sleep";
    //DOUT("processEvent: enter");
    //qDebug() << "player::closeEvent:eventloop:enter";
    //QEventLoop loop;
    //connect(this, SIGNAL(disposed()), &loop, SLOT(quit()), Qt::QueuedConnection);
    //connect(this, SIGNAL(stopped()), &loop, SLOT(quit()), Qt::QueuedConnection);
    //connect(this, SIGNAL(mediaChanged()), &loop, SLOT(quit()), Qt::QueuedConnection);
    //loop.processEvents();
    //qDebug() << "player::closeEvent:eventloop:leave";

    qApp->processEvents(QEventLoop::AllEvents, ProcessEventsTimeout);
  }

  d_->setPaused(false);
  d_->setSubtitleId();
  d_->setTitleId();
  d_->setMediaPath();
  d_->setMediaSize();
  d_->setMediaTitle();
  d_->setTrackNumber();
  d_->setExternalSubtitles();

  d_->trackInfo().clear();;

#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::closeSession();
  VlcHttpPlugin::setMediaTitle(QString());
  VlcHttpPlugin::setUrls(QStringList());
  VlcHttpPlugin::setDuration(0);
#endif // WITH_LIB_VLCHTTP

  if (!d_->mediaList().isEmpty()) {
    foreach (libvlc_media_t *m, d_->mediaList())
      if (m && m != d_->media())
        ::libvlc_media_release(m);
    d_->setMediaList();
  }

  if (d_->media()) {
    ::libvlc_media_release(d_->media());
    d_->setMedia();
  }
  ::libvlc_media_player_set_media(d_->player(), 0); // If any, previous md will be released.

  emit mediaClosed();
  DOUT("exit");
}

bool
Player::hasMedia() const
{
  return isValid() && d_->media();
         //::libvlc_media_player_get_media(d_->player()); // avoid parallel contention
}

bool
Player::hasRemoteMedia() const
{ return hasMedia() && mediaPath().contains("p://"); }

// - Full screen -
void
Player::setFullScreen(bool t)
{
  Q_ASSERT(isValid());
  ::libvlc_set_fullscreen(d_->player(), t);
}

bool
Player::isFullScreen() const
{
  Q_ASSERT(isValid());
  return ::libvlc_get_fullscreen(d_->player());
}

void
Player::toggleFullScreen()
{
  Q_ASSERT(isValid());
  ::libvlc_toggle_fullscreen(d_->player());
}

// - Media information -

QString
Player::mediaTitle() const
{
  //Q_ASSERT(isValid());
  if (!d_->mediaTitle().isEmpty())
    return d_->mediaTitle();

  if (!hasMedia())
    return QString();

  const char *title = ::libvlc_media_get_meta(d_->media(), libvlc_meta_Title);
  // VLC i18n bug. It simpily cannot handle UTF-8 correctly.
  if (!title || ::strstr(title, "??"))
    return QString();

  return _qs(title);
}

QString
Player::mediaPath() const
{
  Q_ASSERT(isValid());
  return d_->mediaPath();
}

Player::MediaInfo
Player::mediaInfo() const
{
  MediaInfo ret;
  if (hasMedia()) {
    ret.path = mediaPath();
    ret.title = mediaTitle();
    ret.track = trackNumber();
  }
  return ret;
}

int
Player::trackNumber() const
{ return d_->trackNumber(); }

int
Player::trackCount() const
{ return d_->mediaList().size(); }

bool
Player::hasPlaylist() const
{ return !d_->mediaList().isEmpty(); }

QList<Player::MediaInfo>
Player::playlist() const
{
  QList<MediaInfo> ret;
  int i = 0;
  foreach (libvlc_media_t *md, d_->mediaList()) {
    MediaInfo mi;
    mi.track = i++;

    const char *mrl = ::libvlc_media_get_mrl(md);
    if (mrl)
      mi.path = mrl;

    const char *title = ::libvlc_media_get_meta(md, libvlc_meta_Title);
    if (title && !::strstr(title, "??"))
      mi.title = title;

    ret.append(mi);
  }
  return ret;
}

// - Play control -

void
Player::play()
{
  DOUT("enter");
  if (hasMedia()) {
    d_->setPaused(false);
    ::libvlc_media_player_play(d_->player());
  }
  DOUT("exit");
}

void
Player::stop()
{
  DOUT("enter");
  if (hasMedia()) {
    emit stopping();
    d_->setPaused(false);
    ::libvlc_media_player_stop(d_->player());
  }
  DOUT("exit");
}

void
Player::pause()
{
  DOUT("enter");
  if (hasMedia() && pausable()) {
    d_->setPaused(true);
    ::libvlc_media_player_set_pause(d_->player(), true);
  }
  DOUT("exit");
}

void
Player::resume()
{
  DOUT("enter");
  if (hasMedia() && pausable()) {
    d_->setPaused(false);
    ::libvlc_media_player_set_pause(d_->player(), false);
  }
  DOUT("exit");
}

void
Player::playPause()
{
  DOUT("enter");
  if (hasMedia() && pausable()) {
    d_->setPaused(!d_->isPaused());
    ::libvlc_media_player_pause(d_->player());
  }
  DOUT("exit");
}

void
Player::nextFrame()
{
  DOUT("enter");
  if (hasMedia() && pausable()) {
    d_->setPaused();
    ::libvlc_media_player_next_frame(d_->player());
  }
  DOUT("exit");
}

bool
Player::snapshot(const QString &path)
{
  DOUT("enter");
  if (!hasMedia()) {
    DOUT("no media, ignored");
    return false;
  }

  int err = ::libvlc_video_take_snapshot(d_->player(), 0, detail::vlcpath(path), 0, 0);
  DOUT("exit: ret =" << !err);
  return !err;
}


// - Set/get properties -

void
Player::setMouseEnabled(bool enable)
{
  Q_ASSERT(isValid());
  d_->setMouseEnabled(enable);
  ::libvlc_video_set_mouse_input(d_->player(), enable ? 1 : 0);
}

bool
Player::isMouseEnabled() const
{
  Q_ASSERT(isValid());
  return d_->isMouseEnabled();
}

void
Player::setKeyboardEnabled(bool enable)
{
  Q_ASSERT(isValid());
  d_->setKeyEnabled(enable);
  ::libvlc_video_set_key_input(d_->player(), enable ? 1 : 0);
}

bool
Player::isKeyboardEnabled() const
{
  Q_ASSERT(isValid());
  return d_->isKeyEnabled();
}

void
Player::mute()
{ setVolume(0); }

void
Player::setVolume(qreal vol)
{
  DOUT("enter");
  Q_ASSERT(isValid());
  if (d_) {
    int newv = vol * VLC_MAX_VOLUME;
    int oldv = ::libvlc_audio_get_volume(d_->player());
    if (oldv != newv) {
      ::libvlc_audio_set_volume(d_->player(), newv);
      emit volumeChanged();
    } else if (newv == 0 || newv == VLC_MAX_VOLUME)
      emit volumeChanged();
  }
  DOUT("exit");
}

void
Player::setPosition(qreal pos, bool checkPos)
{
  DOUT("enter");
  if (hasMedia()) {
    if (checkPos) {
      double max = availablePosition();
      if (!qFuzzyCompare(1 + max, 1) && pos > max) {
        emit errorMessage(tr("seek too much"));
        return;
      }
    }
    ::libvlc_media_player_set_position(d_->player(), pos);
    emit seeked();
  }
  DOUT("exit");
}

QString
Player::aspectRatio() const
{
  Q_ASSERT(isValid());
  char *p = ::libvlc_video_get_aspect_ratio(d_->player());
  QString ret;
  if (p) {
    ret = _qs(p);
    ::libvlc_free(p);
  }
  return ret;
}

void
Player::setAspectRatio(const char *ratio)
{
  Q_ASSERT(isValid());
  ::libvlc_video_set_aspect_ratio(d_->player(), ratio);
  emit aspectRatioChanged(_qs(ratio));
}
void
Player::setAspectRatio(const QString &ratio)
{
  Q_ASSERT(isValid());
  if (ratio.isEmpty())
    ::libvlc_video_set_aspect_ratio(d_->player(), 0);
  else
    ::libvlc_video_set_aspect_ratio(d_->player(), _cs(ratio));
  emit aspectRatioChanged(ratio);
}

void
Player::clearAspectRatio()
{
  Q_ASSERT(isValid());
  ::libvlc_video_set_aspect_ratio(d_->player(), 0);
  emit aspectRatioChanged(QString());
}

void
Player::setAspectRatio(int width, int height)
{
  if (width > 0 && height > 0)
    setAspectRatio(QString("%1:%2").arg(QString::number(width)).arg(QString::number(height)));
  else
    clearAspectRatio();

}

void
Player::setRate(qreal rate)
{
  Q_ASSERT(isValid());
  ::libvlc_media_player_set_rate(d_->player(), rate);
  emit rateChanged(rate);
}

qreal
Player::rate() const
{
  Q_ASSERT(isValid());
  return ::libvlc_media_player_get_rate(d_->player());
}

qreal
Player::fps() const
{
  Q_ASSERT(isValid());
  return ::libvlc_media_player_get_fps(d_->player());
}

qreal
Player::volume() const
{
  Q_ASSERT(isValid());
  return ::libvlc_audio_get_volume(d_->player()) / qreal(VLC_MAX_VOLUME);
}

qreal
Player::position() const
{ return hasMedia() ? ::libvlc_media_player_get_position(d_->player()) : 0; }

qreal
Player::availablePosition() const
{
  if (!hasMedia())
    return 0;
  //if (!hasRemoteMedia())
  //  return 0;

#ifdef WITH_LIB_VLCHTTP
  qint64 duration, size;
  if ((duration = VlcHttpPlugin::duration()) > 0) {
    qint64 ts = VlcHttpPlugin::availableDuration();
    return ts / (double)duration;
  } else if ((size = VlcHttpPlugin::size()) > 0) {
    qint64 sz = VlcHttpPlugin::availableSize();
    return sz / (double)size;
  } else
#endif // WITH_LIB_VLCHTTP
    return 0;
}

// - Play time -

qint64
Player::mediaSize() const
{
  Q_ASSERT(isValid());
  qint64 ret = d_->mediaSize();
#ifdef WITH_LIB_VLCHTTP
  if (!ret && d_->mediaPath().startsWith("http://"))
    ret = VlcHttpPlugin::size();
#endif // WITH_LIB_VLCHTTP
  return ret;
}

qint64
Player::mediaLength() const
{
  Q_ASSERT(isValid());
  return ::libvlc_media_player_get_length(d_->player());
}

qint64
Player::time() const
{
  Q_ASSERT(isValid());
  return ::libvlc_media_player_get_time(d_->player());
}

void
Player::setTime(qint64 time)
{
  DOUT("enter");
  Q_ASSERT(isValid());
  if (hasMedia()) {
    ::libvlc_media_player_set_time(d_->player(), time);
    emit seeked();
  }
  DOUT("exit");
}

// - Subtitles -

int
Player::subtitleCount() const
{
  Q_ASSERT(isValid());
  return hasMedia() ? ::libvlc_video_get_spu_count(d_->player())
                    : 0;
}

bool
Player::hasSubtitles() const
{
  Q_ASSERT(isValid());
  return subtitleCount() > 0;
}

QStringList
Player::subtitleDescriptions() const
{
  Q_ASSERT(isValid());
  QStringList ret;

  libvlc_track_description_t *first = ::libvlc_video_get_spu_description(d_->player());

  /// Skip first disabled track.
  if (first)
    first = first->p_next;

  while (first) {
    if (first->psz_name)
      ret.append(first->psz_name);
    else
      ret.append(QString());
    first = first->p_next;
  }

  return ret;
}

int
Player::subtitleId() const
{
  Q_ASSERT(isValid());
  int id = ::libvlc_video_get_spu(d_->player());
  return id > 0 ? id - 1 :
         d_->subtitleId() - 1;
}

bool
Player::isSubtitleVisible() const
{
  Q_ASSERT(isValid());
  return ::libvlc_video_get_spu(d_->player()) > 0;
}

void
Player::setSubtitleVisible(bool visible)
{
  Q_ASSERT(isValid());
  if (visible) {
    ::libvlc_video_set_spu(d_->player(), d_->subtitleId());
  } else {
    int bak = ::libvlc_video_get_spu(d_->player());
    if (bak > 0) {
      d_->setSubtitleId(bak);
      ::libvlc_video_set_spu(d_->player(), 0);
    }
  }
}

void
Player::showSubtitle()
{ setSubtitleVisible(true); }

void
Player::hideSubtitle()
{ setSubtitleVisible(false); }

void
Player::setSubtitleId(int id)
{
  Q_ASSERT(isValid());
  Q_ASSERT(0 <= id && id < subtitleCount());
  bool err = ::libvlc_video_set_spu(d_->player(), id + 1);
  if (!err) {
    d_->setSubtitleId(id + 1);
    emit subtitleChanged();
  } else
    DOUT("failed: id =" << id);
}

bool
Player::openSubtitle(const QString &fileName)
{ return setSubtitleFromFile(fileName); }

bool
Player::setSubtitleFromFile(const QString &fileName)
{
  DOUT("enter:" << fileName);
  Q_ASSERT(isValid());

  QString path = fileName;
#ifdef Q_OS_WIN
  path.replace('/', '\\');
#endif // Q_OS_WIN

  DOUT("opening subtitle:" << path);
  if (d_->externalSubtitles().contains(path)) {
    DOUT("subtitle already loaded");
    DOUT("exit");
    return true;
  }

  bool ok = ::libvlc_video_set_subtitle_file(d_->player(), detail::vlcpath(path));
  if (ok) {
    DOUT("succeeded, number of subtitles (0 for the first time):" << ::libvlc_video_get_spu_count(d_->player()));
    d_->externalSubtitles().append(path);
    d_->setSubtitleId(::libvlc_video_get_spu_count(d_->player()) - 1);
    emit subtitleChanged();
  } else
    DOUT("failed");

  DOUT("exit");
  return ok;
}

void
Player::addSubtitleFromFile(const QString &fileName)
{
  Q_ASSERT(isValid());
  int bak = ::libvlc_video_get_spu(d_->player());
  if (bak > 0)
    d_->setSubtitleId(bak);
  ::libvlc_video_set_subtitle_file(d_->player(), detail::vlcpath(fileName));
  ::libvlc_video_set_spu(d_->player(), d_->subtitleId());
}

void
Player::loadExternalSubtitles()
{
  QStringList subtitles = searchExternalSubtitles();
  if (!subtitles.empty()) {
    foreach (const QString &path, subtitles)
      openSubtitle(path);

    if (hasSubtitles())
      setSubtitleId(0);
  }
}

QStringList
Player::searchMediaSubtitles(const QString &fileName)
{
  QStringList ret;
  QFileInfo fi(fileName);
  QDir dir = fi.absoluteDir();
  if (dir.exists()) {
    QString baseName = fi.completeBaseName();
    QStringList filters = supportedSubtitleFilters();
    foreach (const QString &f, dir.entryList(filters, QDir::Files))
      if (f.startsWith(baseName + "."))
        ret.append(dir.absolutePath() + "/" + f);
  }
  return ret;
}

QString
Player::searchSubtitleMedia(const QString &fileName)
{
  QFileInfo fi(fileName);
  QDir dir = fi.absoluteDir();
  if (dir.exists()) {
    QString baseName = fi.baseName() ;
    QStringList filters = supportedVideoFilters();
    foreach (const QString &f, dir.entryList(filters, QDir::Files))
      if (f.startsWith(baseName + "."))
        return fi.absolutePath() + "/" + f;
  }
  return QString();
}

QStringList
Player::searchExternalSubtitles() const
{
  DOUT("enter");
  QStringList ret;
  if (hasMedia())
    ret = searchMediaSubtitles(mediaPath());
  DOUT("exit: count:" << ret.size());
  return ret;
}

// - Title/chapter -

bool
Player::hasTitles() const
{ return titleCount() > 0; }

int
Player::titleId() const
{
  DOUT("enter");
  int ret = 0;
  if (hasMedia())
    ret = ::libvlc_media_player_get_title(d_->player());
  DOUT("exit: ret =" << ret);
  return ret;
}

int
Player::titleCount() const
{
  DOUT("enter");
  int ret = 0;
  if (hasMedia())
    ret = ::libvlc_media_player_get_title_count(d_->player());
  DOUT("exit: ret =" << ret);
  return ret;
}

void
Player::setTitleId(int tid)
{
  DOUT("enter: tid =" << tid);
  if (hasMedia()) {
    ::libvlc_media_player_set_title(d_->player(), tid);
    updateTitleId();
  }
  DOUT("exit");
}

void
Player::setPreviousTitle()
{
  DOUT("enter");
  if (hasMedia()) {
    int tid = titleId();
    if (tid > 0)
      setTitleId(tid - 1);
  }
  DOUT("exit");
}

void
Player::setNextTitle()
{
  DOUT("enter");
  if (hasMedia()) {
    int tid = titleId();
    if (tid < titleCount() - 1)
      setTitleId(tid + 1);
  }
  DOUT("exit");
}

void
Player::updateTitleId()
{
  DOUT("ienter");
  if (hasMedia()) {
    int tid = titleId();
    if (tid != d_->titleId()) {
      d_->setTitleId(tid);
      emit titleIdChanged(tid);
    }
  }
  DOUT("exit");
}

bool
Player::hasChapters() const
{ return chapterCount() > 0; }

int
Player::chapterId() const
{
  DOUT("enter");
  int ret = 0;
  if (hasMedia())
    ret = ::libvlc_media_player_get_chapter(d_->player());
  DOUT("exit: ret =" << ret);
  return ret;
}

int
Player::chapterCount() const
{
  DOUT("enter");
  int ret = 0;
  if (hasMedia())
    ret = ::libvlc_media_player_get_chapter_count(d_->player());
  DOUT("exit: ret =" << ret);
  return ret;
}

int
Player::chapterCountForTitleId(int tid) const
{
  DOUT("enter: tid =" << tid);
  int ret = 0;
  if (hasMedia())
    ret = ::libvlc_media_player_get_chapter_count_for_title(d_->player(), tid);
  DOUT("exit: ret =" << ret);
  return ret;
}

void
Player::setChapterId(int cid)
{
  DOUT("enter: cid =" << cid);
  if (hasMedia())
    ::libvlc_media_player_set_chapter(d_->player(), cid);
  DOUT("exit");
}

void
Player::setPreviousChapter()
{
  DOUT("enter");
  if (hasMedia())
    ::libvlc_media_player_previous_chapter(d_->player());
  DOUT("exit");
}

void
Player::setNextChapter()
{
  DOUT("enter");
  if (hasMedia())
    ::libvlc_media_player_next_chapter(d_->player());
  DOUT("exit");
}

// - vlccore -

bool
Player::isMouseEventEnabled() const
{ return d_->isMouseEventEnabled(); }

void
Player::setMouseEventEnabled(bool enabled)
{
  d_->setMouseEventEnabled(enabled);
  //if (enabled)
  //  startVoutTimer();
  //else
  //  stopVoutTimer();
}

void
Player::startVoutTimer()
{
  QxCountdownTimer *timer = d_->voutCountdown();
  if (!timer) {
    d_->setVoutCountdown(
      timer = new QxCountdownTimer(this)
    );
    timer->setInterval(VOUT_TIMEOUT);
    connect(timer, SIGNAL(timeout()), SLOT(updateVout()));
  }

  if (!timer->isActive())
    timer->start(VOUT_COUNTDOWN);
  else
    timer->setCount(VOUT_COUNTDOWN);
}

void
Player::stopVoutTimer()
{
  if (d_->voutCountdown())
    d_->voutCountdown()->stop();
}

void
Player::updateVout()
{
  if (hasMedia()) {
#ifdef WITH_LIB_VLCCORE
    bool ok = ::register_vout_callbacks_(d_->player(), this);
    if (ok) // new vout found
#endif // WITH_LIB_VLCCORE
      stopVoutTimer();
  }
}

// - Error handling -

void
Player::handleError()
{
  // Handle subitems of CDDA.
  // See: http://forum.videolan.org/viewtopic.php?f=32&t=88981
  if (d_->media() &&
      ::libvlc_media_subitems(d_->media())) {

    QList<libvlc_media_t *> l = parsePlaylist(d_->media());
    if (!l.isEmpty()) {
      d_->setMediaList(l);
      //setTrackNumber(d_->trackNumber());
      QTimer::singleShot(0, this, SLOT(setTrackNumber()));
      return;
    }
  }

  emit stopped();
  //closeMedia();
  emit mediaClosed();

  emit errorEncountered();
}

// - Playlist -

QList<libvlc_media_t *>
Player::parsePlaylist(const QString &fileName) const
{
  QList<libvlc_media_t *> ret;
#ifdef USE_PLAYER_PLAYLIST
  QFileInfo fi(fileName);
  if (!fi.exists())
    return ret;

  // jichi 11/30/2011 FIXME: ml never released that could cause runtime memory leak.
  libvlc_instance_t *parent = const_cast<libvlc_instance_t *>(d_->instance());
  libvlc_media_list_t *ml = ::libvlc_media_list_new(parent);

  ::libvlc_media_list_add_file_content(ml, detail::vlcpath(fileName)); // FIXME: deprecated in VLC 1.1.11
  int count = ::libvlc_media_list_count(ml);
  Q_ASSERT(count == 1);
  for (int i = 0; i < count; i++) { // count should always be 1 if succeed
    libvlc_media_t *md = ::libvlc_media_list_item_at_index(ml, i);
    Q_ASSERT(md);
    ret.append(parsePlaylist(md));
  }

  ::libvlc_media_list_release(ml);

#else
  Q_UNUSED(fileName)
#endif // USE_PLAYER_PLAYLIST
  return ret;
}

QList<libvlc_media_t *>
Player::parsePlaylist(libvlc_media_t *md) const
{
  QList<libvlc_media_t *> ret;
  Q_ASSERT(md);
  if (!md)
    return ret;

  libvlc_media_list_t *ml = ::libvlc_media_subitems(md);
  if (ml) {
    int count = ::libvlc_media_list_count(ml);
    for (int i = 0; i < count; i++)
      ret.append(
        ::libvlc_media_list_item_at_index(ml, i)
      );
  }

  return ret;
}

void
Player::setTrackNumber()
{ setTrackNumber(d_->trackNumber()); }

void
Player::setTrackNumber(int track)
{
  Q_ASSERT(isValid());
  if (track < 0 || track >= trackCount())
    return;

  if (hasMedia())
    stop();

  d_->setTrackNumber(track);
  d_->setMedia(d_->mediaList().at(track));
  const char *mrl = ::libvlc_media_get_mrl(d_->media());
  d_->setMediaPath(_qs(mrl));

  ::libvlc_media_player_set_media(d_->player(), d_->media());

  if (isMouseEventEnabled())
    startVoutTimer();
  emit trackNumberChanged(track);
}

void
Player::nextTrack()
{
  int track = d_->trackNumber() + 1;
  if (track < 0 || track >= trackCount())
    track = 0;

  setTrackNumber(track);
}

void
Player::previousTrack()
{
  int track = d_->trackNumber() - 1;
  if (track < 0 || track >= trackCount())
    track = 0;

  setTrackNumber(track);
}

// - Audio tracks -

int
Player::audioTrackCount() const
{
  Q_ASSERT(isValid());
  int ret = ::libvlc_audio_get_track_count(d_->player()) - 1;
  if (ret < 0)
    ret = 0;
  return ret;
}

bool
Player::hasAudioTracks() const
{
  Q_ASSERT(isValid());
  return audioTrackCount() > 1;
}

QStringList
Player::audioTrackDescriptions() const
{
  Q_ASSERT(isValid());
  QStringList ret;

  libvlc_track_description_t *first = ::libvlc_audio_get_track_description(d_->player());

  /// Skip first disabled track.
  if (first)
    first = first->p_next;

  while (first) {
    ret.append(_qs(first->psz_name));
    first = first->p_next;
  }

  return ret;
}

int
Player::audioTrackId() const
{
  Q_ASSERT(isValid());
  int ret = ::libvlc_audio_get_track(d_->player()) - 1;
  if (ret < 0)
    ret = 0;
  return ret;
}

void
Player::setAudioTrackId(int id)
{
  Q_ASSERT(isValid());
  Q_ASSERT(0 <= id && id < audioTrackCount());
  bool ok = ::libvlc_audio_set_track(d_->player(), id + 1);
  if (ok)
    emit audioTrackChanged();
}

// - Audio Channel -

Player::AudioChannel
Player::audioChannel() const
{
  Q_ASSERT(isValid());
  switch (::libvlc_audio_get_channel(d_->player())) {
  case libvlc_AudioChannel_Stereo:  return StereoChannel;
  case libvlc_AudioChannel_RStereo: return ReverseStereoChannel;
  case libvlc_AudioChannel_Left:    return LeftChannel;
  case libvlc_AudioChannel_Right:   return RightChannel;
  case libvlc_AudioChannel_Dolbys:  return DolbysChannel;
  case libvlc_AudioChannel_Error:
  default: return NoChannel;
  }
}

void
Player::setAudioChannel(int channel)
{
  Q_ASSERT(isValid());
  Q_ASSERT(channel > 0 && channel <= DolbysChannel);
  channel = qBound<int>(StereoChannel, channel, DolbysChannel);
  if (channel != audioChannel()) {
    ::libvlc_audio_set_channel(d_->player(), channel);
    emit audioChannelChanged(audioChannel());
  }
}

qint64
Player::audioDelay() const
{
  Q_ASSERT(isValid());
  return ::libvlc_audio_get_delay(d_->player());
}

void
Player::setAudioDelay(qint64 msecs)
{
  Q_ASSERT(isValid());
  if (hasMedia()) {
    ::libvlc_audio_set_delay(d_->player(), msecs);
    emit audioDelayChanged(audioDelay());
  }
}

// - User Agent -

QString
Player::defaultUserAgent()
{ return PLAYER_USER_AGENT; }

QString
Player::userAgent() const
{
  Q_ASSERT(isValid());
  return d_->userAgent();
}

void
Player::setUserAgent(const QString &agent)
{
  Q_ASSERT(isValid());
  if (agent.isEmpty()) {
    d_->setUserAgent(PLAYER_USER_AGENT);
    ::libvlc_set_user_agent(d_->instance(), PLAYER_USER_AGENT, PLAYER_USER_AGENT);
  } else {
    d_->setUserAgent(agent);
    ::libvlc_set_user_agent(d_->instance(), PLAYER_USER_AGENT, _cs(d_->userAgent()));
  }
}

void
Player::setCookieJar(QNetworkCookieJar *jar)
{
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::setNetworkCookieJar(jar);
#else
  Q_UNUSED(jar)
#endif // WITH_LIB_VLCHTTP
}

bool
Player::isBufferSaved() const
{
#ifdef WITH_LIB_VLCHTTP
  return VlcHttpPlugin::isBufferSaved();
#else
  return false;
#endif // WITH_LIB_VLCHTTP
}

bool
Player::isDownloadFinished() const
{
#ifdef WITH_LIB_VLCHTTP
  return VlcHttpPlugin::isFinished();
#else
  return false;
#endif // WITH_LIB_VLCHTTP
}

QString
Player::downloadPath() const
{
#ifdef WITH_LIB_VLCHTTP
  return VlcHttpPlugin::cacheDirectory();
#else
  return QString();
#endif // WITH_LIB_VLCHTTP
}

void
Player::setDownloadsLocation(const QString &dir)
{
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::setCacheDirectory(dir);
#else
  Q_UNUSED(dir)
#endif // WITH_LIB_VLCHTTP
}

void
Player::setBufferSaved(bool t)
{
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::setBufferSaved(t);
#else
  Q_UNUSED(t)
#endif // WITH_LIB_VLCHTTP
}

void
Player::saveBuffer()
{
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::save();
#endif // WITH_LIB_VLCHTTP
}

// - Title -

void
Player::setMediaTitle(const QString &t)
{
  Q_ASSERT(isValid());
  if (t != d_->mediaTitle()) {
    d_->setMediaTitle(t);
#ifdef WITH_LIB_VLCHTTP
    VlcHttpPlugin::setMediaTitle(t);
#endif // WITH_LIB_VLCHTTP
    emit mediaTitleChanged(t);
  }
}

// - Dispose

void
Player::dispose()
{
  emit disposed();
  //if (d_)
  //  detachEvents();

  if (hasMedia()) {
    if (!isStopped())
      mute();
    closeMedia();
  }
#ifdef WITH_LIB_VLCHTTP
  VlcHttpPlugin::unload();
#endif // WITH_LIB_VLCHTTP
}

// - Adjustment -

//bool
//Player::isAdjustEnabled() const
//{
//  Q_ASSERT(isValid());
//  return ::libvlc_video_get_adjust_int(d_->player(), libvlc_adjust_Enable);
//}
//
//void
//Player::setAdjustEnabled(bool t)
//{
//  Q_ASSERT(isValid());
//  int value = t ? 1 : 0;
//  ::libvlc_video_set_adjust_int(d_->player(), libvlc_adjust_Enable, value);
//  emit adjustEnabledChanged(t);
//}

#define MAKE_ADJUST(_adjust, _Adjust, _type, _suffix) \
  _type \
  Player::_adjust() const \
  { \
    Q_ASSERT(isValid()); \
    return ::libvlc_video_get_adjust_##_suffix(d_->player(), libvlc_adjust_##_Adjust); \
  } \
  void \
  Player::set##_Adjust(_type value) \
  { \
    Q_ASSERT(isValid()); \
    ::libvlc_video_set_adjust_int(d_->player(), libvlc_adjust_Enable, 1); \
    ::libvlc_video_set_adjust_##_suffix(d_->player(), libvlc_adjust_##_Adjust, value); \
    emit _adjust##Changed(_adjust()); \
  }

  MAKE_ADJUST(contrast, Contrast, qreal, float)
  MAKE_ADJUST(brightness, Brightness, qreal, float)
  MAKE_ADJUST(hue, Hue, int, int)
  MAKE_ADJUST(saturation, Saturation, qreal, float)
  MAKE_ADJUST(gamma, Gamma, qreal, float)

#undef MAKE_ADJUST

// - Track info -

QSize
Player::videoDimension() const
{
  if (!hasMedia())
    return QSize();

  if (d_->trackInfo().isEmpty())
    const_cast<Self *>(this)->updateTrackInfo();
  return QSize(d_->trackInfo().width, d_->trackInfo().height);
}

QString
Player::codecName(int codecId)
{
#ifdef WITH_LIB_VLCCORE
  return _qs(vlccore::codec_name(codecId));
#else
  return QString::number(codecId);
#endif // WITH_LIB_VLCCORE
}

int
Player::videoCodecId() const
{
  if (!hasMedia())
    return 0;

  if (d_->trackInfo().isEmpty())
    const_cast<Self *>(this)->updateTrackInfo();
  return d_->trackInfo().videoCodecId;
}

int
Player::audioCodecId() const
{
  if (!hasMedia())
    return 0;

  if (d_->trackInfo().isEmpty())
    const_cast<Self *>(this)->updateTrackInfo();
  return d_->trackInfo().audioCodecId;
}

int
Player::audioChannelCount() const
{
  if (!hasMedia())
    return 0;

  if (d_->trackInfo().isEmpty())
    const_cast<Self *>(this)->updateTrackInfo();
  return d_->trackInfo().channels;
}

int
Player::audioRate() const
{
  if (!hasMedia())
    return 0;

  if (d_->trackInfo().isEmpty())
    const_cast<Self *>(this)->updateTrackInfo();
  return d_->trackInfo().rate;
}

void
Player::updateTrackInfo()
{
  Q_ASSERT(isValid());
  detail::TrackInfo &info = d_->trackInfo();
  info.clear();
  if (hasMedia()) {
    libvlc_media_track_info_t *tracks;
    int count = ::libvlc_media_get_tracks_info(d_->media(), &tracks);
    if (count > 0 && tracks) {
      for (int i = 0; i < count; i++)
        switch (tracks[i].i_type) {
        case libvlc_track_video:
          info.videoCodecId = tracks[i].i_codec;
          info.width = tracks[i].u.video.i_width;
          info.height = tracks[i].u.video.i_height;
          break;
        case libvlc_track_audio:
          info.audioCodecId = tracks[i].i_codec;
          info.channels = tracks[i].u.audio.i_channels;
          info.rate = tracks[i].u.audio.i_rate;
          break;
        default: ;
        }
      ::libvlc_free(tracks);
    }
  }
}

// - Media stats -

qreal
Player::bitrate() const
{
  Q_ASSERT(isValid());
  qreal ret = 0;
  if (hasMedia()) {
    libvlc_media_stats_t stats;
    if (::libvlc_media_get_stats(d_->media(), &stats))
      ret = stats.f_input_bitrate * (1000 * 1000);
  }
  return ret;
}

// - Meta data -

#define META_DATA(_Field) \
  QString \
  Player::meta##_Field() const \
  { return hasMedia() ? _qs(::libvlc_media_get_meta(d_->media(), libvlc_meta_##_Field)) : QString(); }

  META_DATA(Title)
  META_DATA(Artist)
  META_DATA(Genre)
  META_DATA(Copyright)
  META_DATA(Album)
  META_DATA(TrackNumber)
  META_DATA(Description)
  META_DATA(Rating)
  META_DATA(Date)
  META_DATA(Setting)
  META_DATA(URL)
  META_DATA(Language)
  META_DATA(NowPlaying)
  META_DATA(Publisher)
  META_DATA(EncodedBy)
  META_DATA(ArtworkURL)
  META_DATA(TrackID)
#undef META_DATA


// EOF

// - PlayerListener -

/*
PlayerListener::PlayerListener(Player *player, QObject *parent)
  : Base(parent), player_(player),
    isPlayingChanged_(false),
    isPausedChaned_(false),
    isStoppedChaned_(false),
    mediaChanged_(false),
    timeChanged_(false),
    lengthChanged_(false),
    positionChanged_(false),
    volumeChanged_(false),
    encodingChanged_(false)
{
  Q_ASSERT(player_);
}

#define CONNECTED_SIGNAL(_signal) \
  bool \
  PlayerListener::connected_##_signal() const \
  { return _signal##_; }

  CONNECTED_SIGNAL(isPlayingChanged)
  CONNECTED_SIGNAL(isPausedChaned)
  CONNECTED_SIGNAL(isStoppedChaned)
  CONNECTED_SIGNAL(mediaChanged)
  CONNECTED_SIGNAL(timeChanged)
  CONNECTED_SIGNAL(lengthChanged)
  CONNECTED_SIGNAL(positionChanged)
  CONNECTED_SIGNAL(volumeChanged)
  CONNECTED_SIGNAL(encodingChanged)

#undef CONNECTED_SIGNAL

void
Player::setUpdateInterval(int milliseconds)
{
  Q_ASSERT(isValid());
  QTimer *timer = d_->timer();
  Q_ASSERT(timer);
  if (timer->isActive())
    timer->stop();

  if (milliseconds > 0)
    timer->start(milliseconds);
}

int
Player::updateInterval() const
{
  Q_ASSERT(isValid());
  return d_->timer()->isActive()? d_->timer()->interval()
                                   : 0;
}
*/
