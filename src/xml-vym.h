#ifndef XML_VYM_H
#define XML_VYM_H

#include "xml-base.h"

#include "vymnote.h"

class BranchItem;
class ImageItem;
class MapItem;
class SlideItem;
class Task;

/*! \brief Parsing VYM maps from XML documents */

class VymReader : public BaseReader {
  public:
    VymReader(VymModel*);

    bool read(QIODevice *device);

  private:
    void raiseUnknownElementError();
    void readVymMap();
    void readSelect();
    void readMapCenter();
    void readBranch();
    void readHeading();
    void readFrame();

    void readVymMapAttr();
    void readBranchAttr();
    void readOrnamentsAttr();
    void readFrameAttr();

    int branchesCounter;
    int branchesTotal;

    VymText vymtext;

    BranchItem *lastBranch;
    //ImageItem *lastImage;
    MapItem *lastMI;
    //SlideItem *lastSlide;
    //Task *lastTask;
    //QString lastSetting;

    bool useProgress;
};





// FIXME-1 Below is mostly obsolete, also in legacy...
/*
enum Content {
    TreeContent = 0x0001,   // FIXME-2 unused
    SlideContent = 0x0002,
    XLinkContent = 0x0004   // FIXME-2 unused
};
*/
#endif
