#ifndef XML_FREEPLANE_H
#define XML_FREEPLANE_H

#include "xml-base.h"

#include <QDebug>

/*! \brief Parsing Freeplane maps */

class BranchItem;

class FreeplaneReader : public BaseReader {
  public:
    FreeplaneReader (VymModel*);
    virtual bool read(QIODevice *device);

  private:
    void foundElement(const QString &e);
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
    void readMapStyles();
    void readNode();
    void readProperties();
    void readRichContent();

    BranchItem* mainBranchRight;
    BranchItem* mainBranchLeft;

    QStringList foundElements;

    QMap <QString, QStringList> elementAttributes;
};

#endif
