#ifndef XMLOBJ_H
#define XMLOBJ_H

class QString;
class QStringList;

QString quoteMeta(const QString &);
QString unquoteMeta(const QString &);
QString quoteQuotes(const QString &);
QString unquoteQuotes(const QString &);
QString getCDATA(const QString &s);

/////////////////////////////////////////////////////////////////////////////
class XMLObj {
  public:
    XMLObj();
    virtual ~XMLObj();
    QString singleElement(QString, QString);         // name,attr
    QString singleElement(QString, QStringList);     // name,attributes
    QString beginElement(QString, QString);          // name,attr
    QString beginElement(QString);                   // name
    QString endElement(QString);                     // name
    QString attribut(QString, QString) const;        // name, val
    QString valueElement(QString, QString);          // name, val
    QString valueElement(QString, QString, QString); // name, val, attr
    void incIndent();
    void decIndent();
    static int curIndent;

  protected:
    QString indent();
    int indentWidth;
};

#endif
