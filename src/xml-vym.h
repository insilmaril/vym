#ifndef XML_VYM_H
#define XML_VYM_H

#include "xml-base.h"

class SlideItem;
class Task;

/*! \brief Parsing VYM maps from XML documents */

class VymReader : public BaseReader {
  public:
    VymReader (VymModel*);
    virtual bool read(QIODevice *device);

  private:
    void raiseUnknownElementError();
    void readVymMap();
    void readSelection();
    void readSetting();
    void readAttribute();
    void readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch);
    void readHeadingOrVymNote();
    void readHtml();
    void readFrame();
    void readLegacyXLink();
    void readStandardFlag();
    void readUserFlagDef();
    void readUserFlag();
    void readImage();
    void readXLink();
    void readSlide();

    void readVymMapAttr();
    void readBranchAttr();
    void readOrnamentsAttr();
    void readFrameAttr();
    void readTaskAttr();

    int branchesCounter;
    int branchesTotal;

    VymText vymtext;

    bool useProgress;
};

#endif
