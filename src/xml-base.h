#ifndef XML_BASE
#define XML_BASE

#include <QXmlStreamReader>

#include "file.h"
#include "vymtext.h"


class VymModel;
class BranchItem;
class ImageItem;
class MapItem;

/*! \brief Base class for parsing maps from XML documents */

class BaseReader {
  public:
    BaseReader(VymModel *vm);
    virtual bool read(QIODevice *device) = 0;

    QString errorString() const;

    QString parseHREF(QString href);
    void setModel(VymModel *);
    void setTmpDir(QString);
    void setInputString(const QString &);
    void setLoadMode(const File::LoadMode &, int p = -1);

  protected:
    VymModel *model;

    QXmlStreamReader xml;

    File::LoadMode loadMode;
    int insertPos;

    QString tmpDir;
    QString inputString;
    QString htmldata;
    QString version;

    VymText vymtext;

    BranchItem *lastBranch;
    //ImageItem *lastImage;
    MapItem *lastMI;
};

#endif
