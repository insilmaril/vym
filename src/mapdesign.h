#ifndef MAPDESIGN_H
#define MAPDESIGN_H

#include <QString>

#include "branch-container.h"
#include "linkobj.h"

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

    LinkObj::ColorHint linkColorHint();
    void setLinkColorHint(const LinkObj::ColorHint &lch);
    QColor defaultLinkColor();
    void setDefaultLinkColor(const QColor &col);

    LinkObj::Style linkStyle(int depth);
    bool setLinkStyle(const LinkObj::Style &style, int depth);

  private:
    QString name;

    QList <Container::Layout> bcNewBranchLayouts;
    QList <Container::Layout> bcRelinkBranchLayouts;

    QList <Container::Layout> icNewBranchLayouts;
    QList <Container::Layout> icRelinkBranchLayouts;

    LinkObj::ColorHint linkColorHintInt;
    QColor defaultLinkCol;

    QList <LinkObj::Style> linkStyles;
};

#endif
