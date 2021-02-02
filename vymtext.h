#ifndef VYMTEXT_H
#define VYMTEXT_H

#include <qcolor.h>
#include <qstring.h>

//  class VymText;

#include "xmlobj.h"

/*! \brief base class for headings of branches and vymnotes */

class VymText : public XMLObj {
  public:
    enum TextMode { AutoText, PlainText, RichText };
    VymText();
    VymText(const VymText &vt);
    VymText(const QString &s);
    bool operator==(const VymText &other);
    void operator=(const VymText &);
    virtual void copy(const VymText &);
    virtual void clear();
    void setText(const QString &);
    void setRichText(const QString &);
    void setPlainText(const QString &);
    void setAutoText(const QString &);
    QString getText() const;
    QString getTextASCII() const;
    QString getTextASCII(QString igdent, const int &width = 0) const;
    void setRichText(bool b);
    bool isRichText() const;
    void setFontHint(const QString &);
    QString getFontHint() const;
    void setFilenameHint(const QString &);
    QString getFilenameHint() const;
    bool isEmpty() const;
    void setColor(QColor color);
    QColor getColor();
    QString getAttributes();
    QString saveToDir();

  protected:
    QString text;
    QString fonthint;
    QString filenamehint;
    TextMode textmode;
    QColor color; // used for plaintext
};
#endif
