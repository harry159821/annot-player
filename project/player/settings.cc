// settings.cc
// 7/30/2011

#include "settings.h"
#include "global.h"
#include "mainwindow.h"
#include "module/annotcloud/traits.h"
#include "module/qtext/algorithm.h"
#include <QtCore>

// - Settings keys -

// See platform specific issue in QSettings manual.
#ifdef Q_OS_MAC
  #define SK_ORGANIZATION   G_DOMAIN
#else
  #define SK_ORGANIZATION   G_ORGANIZATION
#endif // Q_OS_MAC

#define SK_APPLICATION    G_APPLICATION
#define SK_VERSION        "Version"

#define SK_ANNOTLANGUAGES "AnnotLanguages"
#define SK_LIVE         "Live"
#define SK_MENUBAR      "MenuBar"
#define SK_TRANSLATE    "Translate"
#define SK_SUBTITLECOLOR "SubtitleColor"
#define SK_SUBTITLEONTOP "SubtitleOnTop"
#define SK_EMBEDONTOP   "EmbedOnTop"
#define SK_SAVEBUFFER   "SaveBuffer"
#define SK_WINDOWONTOP  "WindowOnTop"
#define SK_UPDATEDATE   "UpdateDate"
#define SK_RECENTPATH   "RecentPath"
#define SK_AUTOSUBMIT   "Submit"
#define SK_ANNOTFILTER  "AnnotationFilter"
#define SK_ANNOTCOUNT   "AnnotationCount"
#define SK_ANNOTEFFECT  "AnnotationEffect"
#define SK_AUTOPLAYNEXT "AutoPlayNext"
#define SK_BLOCKEDUSERS "BlockedUsers"
#define SK_BLOCKEDKEYS  "BlockedKeywords"
#define SK_NICOACCOUNT  "NicovideoAccount"
#define SK_BILIACCOUNT  "BilibiliAccount"
#define SK_PLAYPOSHIST  "PlayPosHistory"
#define SK_SUBTITLEHIST "SubtitleHistory"
#define SK_TRACKHIST    "TrackHistory"
#define SK_ASPECTHIST   "AspectHistory"
#define SK_BROWSERURLS  "BrowserUrls"
#define SK_MULTIWINDOW  "MultipleWindows"
#define SK_QUEUEEMPTY   "QueueEmpty"
#define SK_RECENT       "Recent"

// - Constructions -

Settings::Settings(QObject *parent)
  : Base(
      Base::NativeFormat, Base::UserScope,
      SK_ORGANIZATION, SK_APPLICATION,
      parent)
{ }

// - Properties -

QString
Settings::version() const
{ return value(SK_VERSION).toString(); }

void
Settings::setVersion(const QString &version)
{ setValue(SK_VERSION, version); }

void
Settings::setAnnotationEffect(int id)
{ setValue(SK_ANNOTEFFECT, id); }

int
Settings::annotationEffect() const
{ return value(SK_ANNOTEFFECT).toInt(); }

QDate
Settings::updateDate() const
{ return value(SK_UPDATEDATE).toDate(); }

void
Settings::setUpdateDate(const QDate &date)
{ setValue(SK_UPDATEDATE, date); }

int
Settings::subtitleColor() const
{
  enum { defval = MainWindow::Cyan };
  return value(SK_SUBTITLECOLOR, defval).toInt();
}

void
Settings::setSubtitleColor(int colorId)
{ setValue(SK_SUBTITLECOLOR, colorId); }

qint64
Settings::annotationLanguages() const
{
  enum { defval = AnnotCloud::Traits::AnyLanguageBit };
  return value(SK_ANNOTLANGUAGES, defval).toLongLong();
}

void
Settings::setAnnotationLanguages(qint64 bits)
{ setValue(SK_ANNOTLANGUAGES, bits); }

bool
Settings::isMultipleWindowsEnabled() const
{ return value(SK_MULTIWINDOW).toBool(); }

void
Settings::setMultipleWindowsEnabled(bool t)
{ setValue(SK_MULTIWINDOW, t); }

bool
Settings::isQueueEmpty() const
{ return value(SK_QUEUEEMPTY).toBool(); }

void
Settings::setQueueEmpty(bool empty)
{ setValue(SK_QUEUEEMPTY, empty); }

bool
Settings::isBufferedMediaSaved() const
{ return value(SK_SAVEBUFFER, true).toBool(); }

void
Settings::setBufferedMediaSaved(bool t)
{ setValue(SK_SAVEBUFFER, t); }

bool
Settings::isAutoSubmit() const
{ return value(SK_AUTOSUBMIT, true).toBool(); }

void
Settings::setAutoSubmit(bool t)
{ setValue(SK_AUTOSUBMIT, t); }

bool
Settings::isTranslateEnabled() const
{ return value(SK_TRANSLATE).toBool(); }

void
Settings::setTranslateEnabled(bool enabled)
{ setValue(SK_TRANSLATE, enabled); }

bool
Settings::isSubtitleOnTop() const
{ return value(SK_SUBTITLEONTOP).toBool(); }

void
Settings::setSubtitleOnTop(bool t)
{ setValue(SK_SUBTITLEONTOP, t); }

bool
Settings::isWindowOnTop() const
{ return value(SK_WINDOWONTOP, true).toBool(); }

void
Settings::setWindowOnTop(bool t)
{ setValue(SK_WINDOWONTOP, t); }

bool
Settings::isEmbeddedPlayerOnTop() const
{ return value(SK_EMBEDONTOP).toBool(); }

void
Settings::setEmbeddedPlayerOnTop(bool t)
{ setValue(SK_EMBEDONTOP, t); }

bool
Settings::isAnnotationFilterEnabled() const
{ return value(SK_ANNOTFILTER).toBool(); }

void
Settings::setAnnotationFilterEnabled(bool t)
{ setValue(SK_ANNOTFILTER, t); }

bool
Settings::isAutoPlayNext() const
{ return value(SK_AUTOPLAYNEXT, true).toBool(); }

void
Settings::setAutoPlayNext(bool t)
{ setValue(SK_AUTOPLAYNEXT, t); }

bool
Settings::isMenuBarVisible() const
{ return value(SK_MENUBAR).toBool(); }

void
Settings::setMenuBarVisible(bool t)
{ setValue(SK_MENUBAR, t); }

bool
Settings::isLive() const
{ return value(SK_LIVE).toBool(); }

void
Settings::setLive(bool t)
{ setValue(SK_LIVE, t); }

QString
Settings::recentPath() const
{ return value(SK_RECENTPATH).toString(); }

void
Settings::setRecentPath(const QString &path)
{ setValue(SK_RECENTPATH, path); }

QStringList
Settings::browserUrls() const
{ return value(SK_BROWSERURLS).toStringList(); }

void
Settings::setBrowserUrls(const QStringList &urls)
{
  if (urls.isEmpty())
    remove(SK_BROWSERURLS);
  else
    setValue(SK_BROWSERURLS, urls);
}

void
Settings::clearBrowserUrls()
{ remove(SK_BROWSERURLS); }

QStringList
Settings::recentFiles() const
{
  QStringList ret = value(SK_RECENT).toStringList();

  //QString
  //r = value(SK_RECENT(0)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(1)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(2)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(3)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(4)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(5)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(6)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(7)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(8)).toString(); if (!r.isEmpty()) ret.append(r);
  //r = value(SK_RECENT(9)).toString(); if (!r.isEmpty()) ret.append(r);

  if (!ret.isEmpty())
    ret = QtExt::uniqueList(ret);
  return ret;
}

//void
//Settings::clearRecentFiles()
//{
//  Q_ASSERT(RECENT_COUNT == G_RECENT_COUNT);
//  remove(SK_RECENT(0));
//  remove(SK_RECENT(1));
//  remove(SK_RECENT(2));
//  remove(SK_RECENT(3));
//  remove(SK_RECENT(4));
//  remove(SK_RECENT(5));
//  remove(SK_RECENT(6));
//  remove(SK_RECENT(7));
//  remove(SK_RECENT(8));
//  remove(SK_RECENT(9));
//}

void
Settings::setRecentFiles(const QStringList &l)
{
  if (l.isEmpty())
    remove(SK_RECENT);
  else
    setValue(SK_RECENT, l);

  //QString
  //r = l.size() <= 0 ? QString() : l[0]; setValue(SK_RECENT(0), r);
  //r = l.size() <= 1 ? QString() : l[1]; setValue(SK_RECENT(1), r);
  //r = l.size() <= 2 ? QString() : l[2]; setValue(SK_RECENT(2), r);
  //r = l.size() <= 3 ? QString() : l[3]; setValue(SK_RECENT(3), r);
  //r = l.size() <= 4 ? QString() : l[4]; setValue(SK_RECENT(4), r);
  //r = l.size() <= 5 ? QString() : l[5]; setValue(SK_RECENT(5), r);
  //r = l.size() <= 6 ? QString() : l[6]; setValue(SK_RECENT(6), r);
  //r = l.size() <= 7 ? QString() : l[7]; setValue(SK_RECENT(7), r);
  //r = l.size() <= 8 ? QString() : l[8]; setValue(SK_RECENT(8), r);
  //r = l.size() <= 9 ? QString() : l[9]; setValue(SK_RECENT(9), r);
}

void
Settings::setAnnotationCountHint(int v)
{ setValue(SK_ANNOTCOUNT, v); }

int
Settings::annotationCountHint() const
{ return value(SK_ANNOTCOUNT).toInt(); }

void
Settings::setBlockedKeywords(const QStringList &l)
{
  if (l.isEmpty())
    remove(SK_BLOCKEDKEYS);
  else
    setValue(SK_BLOCKEDKEYS, l);
}

QStringList
Settings::blockedKeywords() const
{ return value(SK_BLOCKEDKEYS).toStringList(); }

void
Settings::setBlockedUserNames(const QStringList &l)
{
  if (l.isEmpty())
    remove(SK_BLOCKEDUSERS);
  else
    setValue(SK_BLOCKEDUSERS, l);
}

QStringList
Settings::blockedUserNames() const
{ return value(SK_BLOCKEDUSERS).toStringList(); }

// - Resume -

QHash<qint64, qint64>
Settings::playPosHistory() const
{
  QHash<qint64, qint64> ret;
  QHash<QString, QVariant> h = value(SK_PLAYPOSHIST).toHash();
  if (!h.isEmpty())
    foreach (QString k, h.keys())
      ret[k.toLongLong()] = h[k].toLongLong();
  return ret;
}

void
Settings::setPlayPosHistory(const QHash<qint64, qint64> &input)
{
  if (input.isEmpty())
    remove(SK_PLAYPOSHIST);
  else {
    QHash<QString, QVariant> h;
    foreach (qint64 k, input.keys())
      h[QString::number(k)] = QString::number(input[k]);
    setValue(SK_PLAYPOSHIST, h);
  }
}

QHash<qint64, int>
Settings::subtitleHistory() const
{
  QHash<qint64, int> ret;
  QHash<QString, QVariant> h = value(SK_SUBTITLEHIST).toHash();
  if (!h.isEmpty())
    foreach (QString k, h.keys())
      ret[k.toLongLong()] = h[k].toInt();
  return ret;
}

void
Settings::setSubtitleHistory(const QHash<qint64, int> &input)
{
  if (input.isEmpty())
    remove(SK_SUBTITLEHIST);
  else {
    QHash<QString, QVariant> h;
    foreach (qint64 k, input.keys())
      h[QString::number(k)] = QString::number(input[k]);
    setValue(SK_SUBTITLEHIST, h);
  }
}

QHash<qint64, int>
Settings::audioTrackHistory() const
{
  QHash<qint64, int> ret;
  QHash<QString, QVariant> h = value(SK_TRACKHIST).toHash();
  if (!h.isEmpty())
    foreach (QString k, h.keys())
      ret[k.toLongLong()] = h[k].toInt();
  return ret;
}

void
Settings::setAudioTrackHistory(const QHash<qint64, int> &input)
{
  if (input.isEmpty())
    remove(SK_TRACKHIST);
  else {
    QHash<QString, QVariant> h;
    foreach (qint64 k, input.keys())
      h[QString::number(k)] = QString::number(input[k]);
    setValue(SK_TRACKHIST, h);
  }
}

QHash<qint64, QString>
Settings::aspectRatioHistory() const
{
  QHash<qint64, QString> ret;
  QHash<QString, QVariant> h = value(SK_ASPECTHIST).toHash();
  if (!h.isEmpty())
    foreach (QString k, h.keys())
      ret[k.toLongLong()] = h[k].toString();
  return ret;
}

void
Settings::setAspectRatioHistory(const QHash<qint64, QString> &input)
{
  if (input.isEmpty())
    remove(SK_PLAYPOSHIST);
  else {
    QHash<QString, QVariant> h;
    foreach (qint64 k, input.keys())
      h[QString::number(k)] = input[k];
    setValue(SK_ASPECTHIST, h);
  }
}

// EOF
