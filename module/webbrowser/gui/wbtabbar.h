#ifndef WBTABBAR_H
#define WBTABBAR_H

// gui/wbtabbar.h
// 4/24/2012

#include <QtGui/QMouseEvent>
#include <QtGui/QTabBar>

class WbTabBar : public QTabBar
{
  Q_OBJECT
  typedef WbTabBar Self;
  typedef QTabBar Base;

public:
  explicit WbTabBar(QWidget *parent = 0)
    : Base(parent) { }

signals:
  void doubleClicked(int index);

  // - Events -
protected:
  virtual void mouseDoubleClickEvent(QMouseEvent *e) ///< \override
  {
    if (e->button() == Qt::LeftButton && !e->modifiers()) {
      emit doubleClicked(tabAt(e->globalPos()));
      e->accept();
    } else
      Base::mouseDoubleClickEvent(e);
  }
};

#endif // WBTABBAR_H