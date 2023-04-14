#ifndef XML_BASE
#define XML_BASE

#include <QXmlStreamReader>

#include "file.h"

#include "task.h"
#include "vymtext.h"


class VymModel;
class BranchItem;
class ImageItem;
class MapItem;
class SlideItem;

/*! \brief Base class for parsing maps from XML documents */

class BaseReader {
  public:
  enum Content {
      TreeContent = 0x0001,   // currently unused
      SlideContent = 0x0002,
      XLinkContent = 0x0004   // currently unused
  };

    BaseReader(VymModel *vm);
    void setContentFilter(const int &);

    virtual bool read(QIODevice *device) = 0;

    QString errorString() const;

    QString parseHREF(QString href);
    void setModel(VymModel *);
    void setTmpDir(QString);
    void setInputString(const QString &);
    void setLoadMode(const File::LoadMode &, int p = -1);
    void raiseUnknownElementError();

    QString attributeToString(const QString &a);

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
    Task *lastTask;
    MapItem *lastMI;

    ImageItem *lastImage;
    SlideItem *lastSlide;

    int contentFilter;
};

#endif
