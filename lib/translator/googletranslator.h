#ifndef GOOGLETRANSLATOR_H
#define GOOGLETRANSLATOR_H

// googletranslator.h
// 6/30/2012

#include "lib/translator/translator.h"
//#include <QtCore/QStack>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QWebPage)

class GoogleTranslator : public Translator
{
  Q_OBJECT
  Q_DISABLE_COPY(GoogleTranslator)
  typedef GoogleTranslator Self;
  typedef Translator Base;

  //QStack<QWebPage *> stack_; // web page pool
  QNetworkAccessManager *nam_; // shared nam
  QWebPage *page_;

  // - Construction -
public:
  explicit GoogleTranslator(QObject *parent = nullptr)
    : Base(parent), nam_(nullptr), page_(nullptr)
  { }

  QString name() const override;

protected slots:
  void doTranslate(const QString &text, int to, int from) override;

protected:
  static QString translateUrl(const QString &text, const char *to, const char *from = 0);
  static const char *lcode(int lang);

  QNetworkAccessManager *networkAccessManager();
  QWebPage *createWebPage();
  //QWebPage *allocateWebPage();
  //void releaseWebPage(QWebPage *page); ///< \internal
public slots:
  void processWebPage(QWebPage *page, bool success); ///< \internal
};

#endif // GOOGLETRANSLATOR_H
