// qtext/draggablewidget.cc
// 9/3/2011

#include "module/qtext/draggablewidget.h"
#include <QtGui>

#define BAD_POS     QPoint(-1, -1)

// - Construction -

QtExt::
DraggableWidget::DraggableWidget(QWidget *parent, Qt::WindowFlags f)
  : Base(parent, f), draggable_(true), dragPos_(BAD_POS)
{ }

// - Events -

void
QtExt::
DraggableWidget::mousePressEvent(QMouseEvent *event)
{
  if (!draggable_) {
    Base::mousePressEvent(event);
    return;
  }

  if (event && event->button() == Qt::LeftButton
      && dragPos_ == BAD_POS) {
    dragPos_ = event->globalPos() - frameGeometry().topLeft();
    event->accept();
  }
}

void
QtExt::
DraggableWidget::mouseMoveEvent(QMouseEvent *event)
{
  if (!draggable_) {
    Base::mouseMoveEvent(event);
    return;
  }

  if (event && event->buttons() & Qt::LeftButton
      && dragPos_ != BAD_POS) {
    QPoint newPos = event->globalPos() - dragPos_;
    move(newPos);
    event->accept();
  }
}

void
QtExt::
DraggableWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (!draggable_) {
    Base::mouseReleaseEvent(event);
    return;
  }

  dragPos_ = BAD_POS;
  if (event)
    event->accept();
}

// EOF
