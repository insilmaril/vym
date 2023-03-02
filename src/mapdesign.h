#ifndef MAPDESIGN_H
#define MAPDESIGN_H

#include <QString>

//#include "branch-container.h"
//#include "branchitem.h"
#include "container.h"
#include "linkobj.h"

/*! \brief A MapDesign defines the visual appearance of a map, e.g. how branches, frames,     links look. Settings depend on
      - depth
      - Mode: NewBranch, RelinkBranch
    Settings may be overriden locallay and locally also depend on other settings, 
    e.g. links may depend on frames
*/

class BranchContainer;
class BranchItem;

/////////////////////////////////////////////////////////////////////////////

class MapDesign {
  public:
    enum HeadingColorHint {
        SpecificColor,
        InheritedColor,
        UnchangedColor,
        UndefinedColor};

    enum UpdateMode {
        NewItem,
        RelinkedItem};

    MapDesign();
    void init();

    void setName(const QString &);
    QString getName();

    Container::Layout branchesContainerLayout(
            const UpdateMode &mode, int depth);
    Container::Layout imagesContainerLayout(
            const UpdateMode &mode, int depth);

    LinkObj::ColorHint linkColorHint();
    void setLinkColorHint(const LinkObj::ColorHint &lch);
    QColor defaultLinkColor();
    void setDefaultLinkColor(const QColor &col);

    LinkObj::Style linkStyle(int depth);
    bool setLinkStyle(const LinkObj::Style &style, int depth);

    void updateBranchHeadingColor(
            BranchItem *branchItem,
            const UpdateMode &mode,
            int depth);

    void updateFrames(
            BranchContainer *branchContainer,
            const UpdateMode &mode,
            int depth);

    QColor selectionColor();
    void setSelectionColor(const QColor &col);

  private:
    QString name;

    QList <Container::Layout> bcNewBranchLayouts;
    QList <Container::Layout> bcRelinkBranchLayouts;

    QList <Container::Layout> icNewBranchLayouts;
    QList <Container::Layout> icRelinkBranchLayouts;

    QList <MapDesign::HeadingColorHint> newBranchHeadingColorHints;
    QList <MapDesign::HeadingColorHint> relinkedBranchHeadingColorHints;
    QList <QColor> newBranchHeadingColors;
    QList <QColor> relinkedBranchHeadingColors;

    LinkObj::ColorHint linkColorHintInt;
    QColor defaultLinkCol;

    QList <LinkObj::Style> linkStyles;

    QColor selectionColorInt;
};

#endif
