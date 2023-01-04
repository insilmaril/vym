#ifndef XML_BASE
#define XML_BASE

#include <QXmlStreamReader>

#include "file.h"

class VymModel;

/*! \brief Base class for parsing maps from XML documents */

class BaseReader {
  public:
    BaseReader(VymModel*);

    QString errorString() const;

    void setLoadMode(const File::LoadMode &lm, int p);

  protected:
    VymModel *model;

    QXmlStreamReader xml;

    File::LoadMode loadMode;
    int insertPos;

    QString htmldata;   // Legacy
    QString version;
};

#endif
