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
    void readVymMap();
    void readMapDesign();
    void readMapDesignElement();
    void readMapDesignCompatibleAttributes();
    void readSelection();
    void readSetting();
    void readAttribute();
    void readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch);
    void readHeadingOrVymNote();
    void readFrame();
    void readLegacyXLink();
    void readStandardFlag();
    void readUserFlagDef();
    void readUserFlag();
    void readImage();
    void readXLink();
    void readSlide();
    void readTask();

    void readVymMapAttr();
    void readBranchAttr();
    void readOrnamentsAttr();
    void readFrameAttr();

    int branchesCounter;
    int branchesTotal;

    VymText vymtext;

    bool useProgress;
};

#endif
