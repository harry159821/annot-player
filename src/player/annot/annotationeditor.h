#ifndef ANNOTATIONEDITOR_H
#define ANNOTATIONEDITOR_H

// annotationeditor.h
// 10/4/2011

#include "src/common/acwindow.h"

QT_BEGIN_NAMESPACE
class QAction;
class QColor;
class QComboBox;
class QFontComboBox;
class QLabel;
class QShortcut;
class QStackedLayout;
class QTextCharFormat;
class QTextEdit;
class QToolButton;
QT_END_NAMESPACE

// TODO: attach image!!!!!!!!!! or URL link as well!!!!

class AnnotationEditor : public AcWindow
{
  Q_OBJECT
  Q_DISABLE_COPY(AnnotationEditor)
  typedef AnnotationEditor Self;
  typedef AcWindow Base;

  enum RibonIndex {
    CodeRibonIndex = 0,
    HtmlRibonIndex,
    RibonIndexCount
  };

protected:
  enum AlignIndex { // See also: http://doc.qt.nokia.com/latest/qt.html#AlignmentFlag-enum
    AlignLeftIndex = 0,
    AlignRightIndex,
    AlignHCenterIndex,
    AlignJustifyIndex,
    AlignIndexCount
  };

  enum MoveStyleIndex {
    MoveStyleFlyIndex = 0,
    MoveStyleTopIndex,
    MoveStyleBottomIndex,
    MoveStyleCount
  };

  enum RenderEffectIndex {
    TransparentEffectIndex = 0,
    ShadowEffectIndex,
    BlurEffectIndex,
    RenderEffectCount
  };

public:
  enum Mode {
    CodeMode = 0,
    HtmlMode,
    ModeCount
  };

public:
  explicit AnnotationEditor(QWidget *parent = nullptr);

signals:
  void textSaved(const QString &text);

  // - Properties
public:
  QString text() const;
  qint64 id() const;

  Mode mode() const;
  bool isCodeMode() const;
  bool isHtmlMode() const;

  bool isTidyEnabled() const;

public slots:
  void setId(qint64 id);
  void setText(const QString &text);
  void clear();

  // - Slots -

protected slots:
  void save();

  void setMode(Mode mode);
  void setCodeMode();
  void setHtmlMode();

  void setSubtitle();

  void setBold(bool t = true); ///< Apply text under cursor
  void setItalic(bool t = true); ///< Apply text under cursor
  void setUnderline(bool t = true); ///< Apply to text under cursor
  void setStrikeOut(bool t = true); ///< Apply to text under cursor

  void setColor(const QColor &color);
  void setBackgroundColor(const QColor &color);
  void setFontType(const QFont &font);
  void setFontFamily(const QString &name);
  void setFontSize(const QString &size);
  void setFormat(const QTextCharFormat &format); ///< Apply to text under cursor
  void clearFormat(); ///< Apply to entire text
  void setAlignment(int alignIndex);

  void setTidyEnabled(bool t);

  void showColorDialog();
  void showBackgroundColorDialog();
  void setColorIconColor(const QColor &c);
  void setBackgroundColorIconColor(const QColor &c);

  void updateAlignment();
  void updateCount();

  void setBlackColor()  { setColor("Black"); }
  void setBlueColor()   { setColor("Blue"); }
  void setBrownColor()  { setColor("Brown"); }
  void setCyanColor()   { setColor("Cyan"); }
  void setGrayColor()   { setColor("Gray"); }
  void setMagentaColor(){ setColor("Magenta"); }
  void setOrangeColor() { setColor("Orange"); }
  void setPinkColor()   { setColor("Pink"); }
  void setRedColor()    { setColor("Red"); }
  void setWhiteColor()  { setColor("White"); }
  void setYellowColor() { setColor("Yellow"); }

  void setVerbatim();
  void setMoveStyle(int moveStyleIndex);
  void setRenderEffect(int effectIndex);

  void addTag(const QString &tag);
  void removeTag(const QString &tag);
  void tag(const QString &tag);

  //void setListStyle(int styleIndex); // LIST: TODO

  // - Events -
protected:
  //void mouseDoubleClickEvent(QMouseEvent *event) override;
  //void contextMenuEvent(QContextMenuEvent *event) override;

  // - Implementations -
protected:
  void mergeFormat(const QTextCharFormat &format); ///< Merge format to text under cursor

private slots:
  // Swap textEdit_->plainText type
  void code2html();
  void html2code();

private:
  void createRibons();
  void createLayout();
  void createActions();

private:
  qint64 id_;
  QString text_;
  QTextEdit *textEdit_;

  // Header
  QStackedLayout *ribonStackLayout_;
  QToolButton *formatButton_;

  // Footer
  QLabel *countLabel_;
  QToolButton *tidyButton_;
  QToolButton *saveButton_,
              *cancelButton_;

  // Code ribon
  QToolButton *codeRibonButton_;
  QComboBox *moveStyleComboBox_,
            *renderEffectComboBox_;
  QToolButton *verbatimButton_,
              *subtitleButton_;

  // Html ribon
  QToolButton *htmlRibonButton_;
  QToolButton *boldButton_,
              *italicButton_,
              *underlineButton_,
              *strikeOutButton_;
  QToolButton *colorButton_,
              *backgroundColorButton_;

  QFontComboBox *fontComboBox_;
  QComboBox *fontSizeComboBox_;
  QComboBox *alignComboBox_;

  // Actions
  QAction *boldAct_,
          *italicAct_,
          *underlineAct_,
          *strikeOutAct_;
  QAction *colorAct_,
          *backgroundColorAct_;

  QShortcut *saveShortcut_;
};

#endif // ANNOTATIONEDITOR_H
