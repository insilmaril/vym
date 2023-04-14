#ifndef XML_FREEPLANE_H
#define XML_FREEPLANE_H

#include "xml-base.h"

#include <QDebug>

/*! \brief Parsing Freeplane maps */

class FreeplaneReader : public BaseReader {
  public:
    FreeplaneReader (VymModel*);
    virtual bool read(QIODevice *device);

  private:
    QString attrString();
    void readToEnd();

    void readArrowLink();
    void readAttribute();
    void readCloud();
    void readEdge();
    void readFont();
    void readIcon();
    void readHook();
    void readMap();
    void readNode();
    void readRichContent();

    VymText vymtext;
};

#endif
