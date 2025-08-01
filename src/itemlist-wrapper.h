#ifndef ITEMLIST_WRAPPER_H
#define ITEMLIST_WRAPPER_H

#include <QObject>

class BranchWrapper;
class VymModel;
class VymModelWrapper;

//Q_DECLARE_METATYPE(ItemListWrapper)
//Q_DECLARE_METATYPE(ItemListWrapper*)

class ItemListWrapper : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE ItemListWrapper (VymModel*);
    ~ItemListWrapper ();
    void init();

public slots:
    Q_INVOKABLE void setModeBranches(bool deepLevelsFirst = false);
    Q_INVOKABLE void setModeSelectedBranches();
    Q_INVOKABLE void setModeSelectedSubtrees(bool deepLevelsFirst = false);
    Q_INVOKABLE BranchWrapper* nextBranch();
    Q_INVOKABLE uint count();

private:    
    VymModel* modelInt;
    QList <uint> itemList;
    int currentIndex;
    bool deepLevelsFirstInt;
};

#endif
