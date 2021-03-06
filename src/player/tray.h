#ifndef TRAY_H
#define TRAY_H

// tray.h
// 10/27/2011

#include "qtx/qxsystemtrayicon.h"

QT_FORWARD_DECLARE_CLASS(QAction)

class MainWindow;

typedef QxSystemTrayIcon TrayBase;
class Tray : public TrayBase
{
  Q_OBJECT
  Q_DISABLE_COPY(Tray)
  typedef Tray Self;
  typedef TrayBase Base;

  // - Constructions -
public:
  explicit Tray(MainWindow *w, QObject *parent = nullptr);

  // - Implementations -
protected slots:
  void activate(QSystemTrayIcon::ActivationReason reason);
  void updateContextMenu();
private:
  void createActions();
private:
  MainWindow *w_;

  QAction *minimizeAct_,
          *restoreAct_,
          *toggleWindowOnTopAct_;
};

#endif // TRAY_H
