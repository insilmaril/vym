#ifndef XSLTPROC_H
#define XSLTPROC_H

#include <qstring.h>
#include <qstringlist.h>

#include "showtextdialog.h"

class XSLTProc {
  public:
    XSLTProc();
    ~XSLTProc();
    void addStringParam(const QString &, const QString &);
    void setOutputFile(const QString &);
    bool setXSLFile(const QString &);
    bool setInputFile(const QString &);
    void addOutput(const QString &);
    bool process();

  private:
    QStringList stringParamKey;
    QStringList stringParamVal;
    QString outputFile;
    QString inputFile;
    QString xslFile;
    QString xsltprocessor;
    bool showOutput;
    ShowTextDialog *dia;
};

#endif
