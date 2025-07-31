#ifndef ITEMLIST_WRAPPER_H
#define ITEMLIST_WRAPPER_H

#include <QObject>

class BranchWrapper;
class VymModel;

//Q_DECLARE_METATYPE(ItemListWrapper)
//Q_DECLARE_METATYPE(ItemListWrapper*)

class ItemListWrapper : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE ItemListWrapper (VymModel*);
    ~ItemListWrapper ();

public slots:
    Q_INVOKABLE BranchWrapper* nextBranch();
    Q_INVOKABLE uint count();

private:    
    VymModel* modelInt;
    QList <uint> itemList;
};

#endif
