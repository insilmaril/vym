#ifndef XML_ITHOUGHTS_H
#define XML_ITHOUGHTS_H

#include "xml-base.h"


/*! \brief Parsing ithoughts maps from XML documents */

class IThoughtsReader : public BaseReader {
  public:
    IThoughtsReader (VymModel*);
    virtual bool read(QIODevice *device);

  private:
    void readIThoughtsMap();
    void readTopics(File::LoadMode loadModeBranch, int insertPosBranch);
    void readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch);

    void readMapAttr();
    void readBranchAttr();
    void readOrnamentsAttr();

    int branchesCounter;
    int branchesTotal;

    VymText vymtext;

    bool useProgress;
};

#endif
