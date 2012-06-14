#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

// messageview.h
// 10/16/2011

#include "processinfo.h"
#include "module/qtext/dialog.h"
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QLabel;
class QSpinBox;
class QTextEdit;
class QToolButton;
QT_END_NAMESPACE

class MessageView : public QtExt::Dialog
{
  Q_OBJECT
  Q_DISABLE_COPY(MessageView)
  typedef MessageView Self;
  typedef QtExt::Dialog Base;

  enum HookIndex {
    HI_AllHooks = 0,
    HI_Count
  };

public:
  explicit MessageView(QWidget *parent = 0);

signals:
  void hookSelected(ulong hookId);

  // - Properties -
public:
   bool isActive() const { return active_; }

   const QString &processName() const { return processName_; }
public slots:
   void setProcessName(const QString &name) { processName_ = name; }
   void clearProcessName() { processName_.clear(); }

   void setProcessNameFromProcessInfo(const ProcessInfo &pi)
   {
     if (pi.isValid())
       setProcessName(pi.processName);
     else
       clearProcessName();
   }

protected:
  ulong currentHookId() const;
  int currentIndex() const;

  // - Actions -

public slots:
  void clear();
  void selectCurrentHook();
  void processHookedText(const QString &text, ulong hookId);

  void setActive(bool active);

  void addMessages(QStringList &messages, ulong hookId);
  void setCurrentIndex(int index);

protected slots:
  void setTextList(const QStringList& text);
  void selectHookIndex(int index);
  void invalidateHookCountLabel();
  void invalidateCurrentCharFormat();
  void invalidateSelectButton();
  void invalidateCurrentHook();

  // - Events -
public:
  virtual void setVisible(bool visible); ///< \reimp

//protected slots:
//  virtual void dragEnterEvent(QDragEnterEvent *event); ///< \reimp
//  virtual void dragMoveEvent(QDragMoveEvent *event); ///< \reimp
//  virtual void dragLeaveEvent(QDragLeaveEvent *event); ///< \reimp
//  virtual void dropEvent(QDropEvent *event); ///< \reimp
//
//signals:
//  void dragEnterEventReceived(QDragEnterEvent *event);
//  void dragMoveEventReceived(QDragMoveEvent *event);
//  void dragLeaveEventReceived(QDragLeaveEvent *event);
//  void dropEventReceived(QDropEvent *event);

  // - Implementation -
protected:
  static bool isBetterHook(ulong goodHookId, ulong badHookId);

private:
  void createLayout();

private:
  bool active_;
  QString processName_;
  QSpinBox *hookIndexEdit_;
  QTextEdit *textEdit_;
  QLabel *hookCountLabel_;

  QVector<ulong> hooks_;
  QVector<QStringList> texts_;

  QToolButton *autoButton_, *selectButton_;
};

#endif // MESSAGEVIEW_H