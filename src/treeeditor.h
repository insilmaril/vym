#ifndef TREEEDITOR_H
#define TREEEDITOR_H

#include <QTreeView>

class VymModel;

/*! \brief TreeView widget in vym to display and edit a map, based on
 * QTreeView */

class TreeEditor : public QTreeView {
    Q_OBJECT

  public:
    TreeEditor(VymModel *m = NULL);
    ~TreeEditor();
    void init();
    QModelIndex getSelectedIndex();

  protected:
    virtual void contextMenuEvent(QContextMenuEvent *e);

  private slots:
    void cursorUp();
    void cursorDown();
    void startEdit();
/*
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &);
*/

  private:
    VymModel *model;
};

#endif
