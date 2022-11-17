#ifndef MAPDESIGN_H
#define MAPDESIGN_H

#include <QString>

#include "branch-container.h"

/*! \brief A MapDesign defines the visual appearance of a map, e.g. how branches, frames,     links look. Settings depend on
      - depth
      - Mode: NewBranch, RelinkBranch
    Settings may be overriden locallay and locally also depend on other settings, 
    e.g. links may depend on frames
*/

/////////////////////////////////////////////////////////////////////////////

class MapDesign {
  public:
    enum OrnamentStyle { None, HeadFull, Foot };
    MapDesign();
    void init();

    void setName(const QString &);
    QString getName();

  public:
    Container::Layout branchesContainerLayout(const BranchContainer::StyleUpdateMode &mode, int depth);
  private:
    QList <Container::Layout> bcNewBranchLayouts;
    QList <Container::Layout> bcRelinkBranchLayouts;

  private:
    QString name;
};

#endif
