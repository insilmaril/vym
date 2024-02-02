#ifndef IMPORTS_H
#define IMPORTS_H

#include <iostream>
#include <QDir>
#include <QProgressDialog>
#include <QString>

#include "settings.h"

class VymModel;
class BranchItem;

///////////////////////////////////////////////////////////////////////

class ImportBase {
  public:
    ImportBase();
    ImportBase(VymModel *m);
    virtual ~ImportBase();
    void init();
    virtual void setDir(const QString &);
    virtual void setFile(const QString &);
    virtual bool transform();
    virtual QString getTransformedFile();

  protected:
    VymModel *model;
    QDir tmpDir;
    QString inputDir;
    QString inputFile;
    QString transformedFile;
};

///////////////////////////////////////////////////////////////////////
class ImportFirefoxBookmarks : public ImportBase {
  public:
    enum ParseMode {countBookmarks, buildMap};
    ImportFirefoxBookmarks(VymModel *m);
    bool transform();
  private:
    QProgressDialog progressDialog;
    bool parseJson(QJsonValue jsval, ParseMode mode, BranchItem *selbi = nullptr);  

    uint totalBookmarks;
    uint currentBookmarks;
};

///////////////////////////////////////////////////////////////////////
class ImportMM : public ImportBase {
  public:
    bool transform();
};

#endif
