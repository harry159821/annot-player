#ifndef COUNTDOWNDIALOG_H
#define COUNTDOWNDIALOG_H

// countdowndialog.h
// 3/13/2012

#include "src/common/acwindow.h"

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QTimer)

class CountdownDialog : public AcWindow
{
  Q_OBJECT
  Q_DISABLE_COPY(CountdownDialog)
  typedef CountdownDialog Self;
  typedef AcWindow Base;

  QLabel *messageLabel_;
  QTimer *timer_; // in seconds
  int count_, currentCount_;
  QString message_;

public:
  enum { DefaultCount = 30 }; ///< in second

  explicit CountdownDialog(QWidget *parent = nullptr);

signals:
  void timeout();

  void message(QString text);
  void errorMessage(QString text);
  void warning(QString text);
  void notification(QString text);

public slots:
  void setCount(int count) { count_ = count; }
  void setText(const QString &text) { message_ = text; } ///< must contain %1

  void setVisible(bool visible) override;

protected slots:
  virtual void run() { emit timeout(); }
  void start();
  void stop();

  void refresh();
  void tick();
  void cancel() { hide(); stop(); }

private:
  void createLayout();
};

#endif // COUNTDOWNDIALOG_H
