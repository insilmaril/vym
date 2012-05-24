#ifndef TASKFILTERMODEL_H
#define TASKFILTERMODEL_H

#include <QSortFilterProxyModel>

class TaskFilterModel:public QSortFilterProxyModel
{
public:
    void setFilter (bool b);
    void setMapFilter (const QString &s);
protected:    
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
private:
    bool useFilter;
    QString mapFilter;
};

#endif
