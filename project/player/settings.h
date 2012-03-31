#ifndef SETTINGS_H
#define SETTINGS_H

// settings.h
// 7/30/2011

#include <QSettings>
#include <QStringList>
#include <QDate>
#include <QHash>
#include <utility>

class Settings : public QSettings
{
  Q_OBJECT
  typedef Settings Self;
  typedef QSettings Base;

  // - Constructions -
public:
  static Self *globalSettings() { static Self g; return &g; }
protected:
  explicit Settings(QObject *parent = 0);

public slots:
  void sync() { Base::sync(); }

  // - Properties -
public:
  QString version() const;
  void setVersion(const QString &version);

  bool isQueueEmpty() const; ///< If database queue is empty
  void setQueueEmpty(bool empty);

  bool isBufferedMediaSaved() const;
  void setBufferedMediaSaved(bool t);

  bool isAutoSubmit() const;
  void setAutoSubmit(bool t);

  bool isTranslateEnabled() const; ///< If use translation in game mode
  void setTranslateEnabled(bool enabled);

  bool isSubtitleOnTop() const;
  void setSubtitleOnTop(bool t);

  bool isWindowOnTop() const;
  void setWindowOnTop(bool t);

  bool isEmbeddedPlayerOnTop() const;
  void setEmbeddedPlayerOnTop(bool t);

  bool isMenuBarVisible() const;
  void setMenuBarVisible(bool t);

  bool isAutoPlayNext() const;
  void setAutoPlayNext(bool t);

  bool isLive() const;
  void setLive(bool t);

  QDate updateDate() const;
  void setUpdateDate(const QDate &date);

  int subtitleColor() const;
  void setSubtitleColor(int colorId);

  int annotationEffect() const;
  void setAnnotationEffect(int effect);

  ///  Theme in UiStyle
  qint64 annotationLanguages() const; ///< Annotation language
  void setAnnotationLanguages(qint64 lang);

  void setMultipleWindowsEnabled(bool t);
  bool isMultipleWindowsEnabled() const;

  QStringList browserUrls() const;
  void setBrowserUrls(const QStringList &urls);
  void clearBrowserUrls();

  QStringList recentFiles() const;
  void setRecentFiles(const QStringList &files);
  //void clearRecentFiles();

  QString recentPath() const;
  void setRecentPath(const QString &path);

  bool isAnnotationFilterEnabled() const;
  void setAnnotationFilterEnabled(bool t);

  QStringList blockedKeywords() const;
  void setBlockedKeywords(const QStringList &l);

  QStringList blockedUserNames() const;
  void setBlockedUserNames(const QStringList &l);

  int annotationCountHint() const;
  void setAnnotationCountHint(int count);

  // - History per token -

  QHash<qint64, qint64> playPosHistory() const;
  void setPlayPosHistory(const QHash<qint64, qint64> &h);

  QHash<qint64, int> subtitleHistory() const;
  void setSubtitleHistory(const QHash<qint64, int> &h);

  QHash<qint64, int> audioTrackHistory() const;
  void setAudioTrackHistory(const QHash<qint64, int> &h);

  QHash<qint64, QString> aspectRatioHistory() const;
  void setAspectRatioHistory(const QHash<qint64, QString> &h);
};

#endif // SETTINGS_H
