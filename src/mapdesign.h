#ifndef MAPDESIGN_H
#define MAPDESIGN_H

#include <QString>

#include "branch-container.h"
#include "link-container.h"

/*! \brief A MapDesign defines the visual appearance of a map, e.g. how branches, frames,     links look. Settings depend on
      - depth
      - Mode: NewBranch, RelinkBranch
    Settings may be overriden locallay and locally also depend on other settings, 
    e.g. links may depend on frames
*/

/////////////////////////////////////////////////////////////////////////////

class MapDesign {
  public:
    MapDesign();
    void init();

    void setName(const QString &);
    QString getName();

    Container::Layout branchesContainerLayout(
            const BranchContainer::StyleUpdateMode &mode, int depth);
    Container::Layout imagesContainerLayout(
            const BranchContainer::StyleUpdateMode &mode, int depth);

    LinkContainer::ColorHint linkColorHint();
    void setLinkColorHint(const LinkContainer::ColorHint &lch);
    QColor defaultLinkColor();
    void setDefaultLinkColor(const QColor &col);

  private:
    QString name;

    QList <Container::Layout> bcNewBranchLayouts;
    QList <Container::Layout> bcRelinkBranchLayouts;

    QList <Container::Layout> icNewBranchLayouts;
    QList <Container::Layout> icRelinkBranchLayouts;

    LinkContainer::ColorHint linkColHint;
    QColor defaultLinkCol;
};

#endif
