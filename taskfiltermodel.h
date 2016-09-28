#ifndef TASKFILTERMODEL_H
#define TASKFILTERMODEL_H

#include <QSortFilterProxyModel>

class TaskFilterModel:public QSortFilterProxyModel
{
public:
    void setFilter (bool b);
    void setFilterNew (bool b);
    void setMapFilter (const QString &s);
    void setFilterFlags (bool b);
protected:    
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private:
    bool useFilter;
    QString mapFilter;
    bool filterNew;
    bool filterFlags;
};

#endif
