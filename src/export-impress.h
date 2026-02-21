#ifndef EXPORT_IMPRESS_H
#define EXPORT_IMPRESS_H

#include "export-base.h"

class ExportImpress : public ExportBase {
  public:
    ExportImpress();
    ~ExportImpress();
    void exportPresentation();
    bool setConfigFile(const QString &);

  private:
    QString buildList(TreeItem *);
    bool useSections;
    QString configFile;
    QString configDir;
    QString templateDir;
    QString content;
    QString contentTemplate;
    QString contentTemplateFile;
    QString contentFile;
    QString pageTemplate;
    QString pageTemplateFile;
    QString sectionTemplate;
    QString sectionTemplateFile;
};

#endif
