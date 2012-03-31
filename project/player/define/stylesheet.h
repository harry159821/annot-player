#ifndef STYLESHEET_H
#define STYLESHEET_H

// stylesheet.h
// 7/15/2011
// See: http://doc.qt.nokia.com/stable/stylesheet-examples.html
// See: http://doc.qt.nokia.com/4.7/stylesheet-reference.html

#include "rc.h"
#include "ac/acss.h"
#include <QtGlobal>

// Tool buttons

#define SS_TOOLBUTTON_(_id) \
  SS_BEGIN(QToolButton) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id) \
  SS_END \
  SS_BEGIN(QToolButton::hover) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id##_HOVER) \
  SS_END \
  SS_BEGIN(QToolButton::pressed) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id##_PRESSED) \
  SS_END \
  SS_BEGIN(QToolButton::checked) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id##_CHECKED) \
  SS_END \
  SS_BEGIN(QToolButton::checked:hover) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id##_CHECKED_HOVER) \
  SS_END \
  SS_BEGIN(QToolButton::disabled) \
    SS_BORDER_IMAGE_URL(RC_IMAGE_##_id##_DISABLED) \
  SS_END

#define SS_TOOLBUTTON_PLAY      SS_TOOLBUTTON_(PLAY)
#define SS_TOOLBUTTON_PAUSE     SS_TOOLBUTTON_(PAUSE)
#define SS_TOOLBUTTON_STOP      SS_TOOLBUTTON_(STOP)
#define SS_TOOLBUTTON_OPEN      SS_TOOLBUTTON_(OPEN)
#define SS_TOOLBUTTON_NEXTFRAME SS_TOOLBUTTON_(NEXTFRAME)
#define SS_TOOLBUTTON_FASTFORWARD SS_TOOLBUTTON_(FASTFORWARD)
#define SS_TOOLBUTTON_FASTFASTFORWARD SS_TOOLBUTTON_(FASTFASTFORWARD)
#define SS_TOOLBUTTON_REWIND    SS_TOOLBUTTON_(REWIND)
#define SS_TOOLBUTTON_MINI      SS_TOOLBUTTON_(MINI)
#define SS_TOOLBUTTON_EMBED     SS_TOOLBUTTON_(EMBED)
#define SS_TOOLBUTTON_FULLSCREEN SS_TOOLBUTTON_(FULLSCREEN)
#define SS_TOOLBUTTON_SNAPSHOT  SS_TOOLBUTTON_(SNAPSHOT)
#define SS_TOOLBUTTON_MENU      SS_TOOLBUTTON_(MENU)
#define SS_TOOLBUTTON_MAXIMIZE  SS_TOOLBUTTON_(MAXIMIZE)
#define SS_TOOLBUTTON_MINIMIZE  SS_TOOLBUTTON_(MINIMIZE)
#define SS_TOOLBUTTON_RESTORE   SS_TOOLBUTTON_(RESTORE)
#define SS_TOOLBUTTON_PREVIOUS  SS_TOOLBUTTON_(PREVIOUS)
#define SS_TOOLBUTTON_NEXT      SS_TOOLBUTTON_(NEXT)

#define SS_TOOLBUTTON_ANNOT     SS_TOOLBUTTON_(ANNOT)
#define SS_TOOLBUTTON_SHOWANNOT SS_TOOLBUTTON_(SHOWANNOT)
#define SS_TOOLBUTTON_HIDEANNOT SS_TOOLBUTTON_(HIDEANNOT)

#define SS_TOOLBUTTON_USER      SS_TOOLBUTTON_TEXT
#define SS_TOOLBUTTON_SEEK      SS_TOOLBUTTON_TEXT

#endif // STYLESHEET_H
