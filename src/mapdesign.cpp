#include "mapdesign.h"

#include <QDebug>

/////////////////////////////////////////////////////////////////
// MapDesign
/////////////////////////////////////////////////////////////////

MapDesign::MapDesign()
{
    //qDebug() << "Constr. MapDesign";
    init();
}

void MapDesign::init()
{
    // NewBranch: Layout of children branches 
    bcNewBranchLayouts << Container::FloatingBounded;
    bcNewBranchLayouts << Container::Vertical;

    // RelinkBranch: Layout of children branches 
    bcRelinkBranchLayouts << Container::FloatingBounded;
    bcRelinkBranchLayouts << Container::Vertical;

    // NewBranch: Layout of children images 
    icNewBranchLayouts << Container::FloatingFree;

    // RelinkBranch: Layout of children images
    icRelinkBranchLayouts << Container::FloatingFree;

    // Heading colors
    newBranchHeadingColorHints << MapDesign::SpecificColor;         // Specific for MapCenter
    newBranchHeadingColorHints << MapDesign::InheritedColor;        // Use color of parent
    relinkedBranchHeadingColorHints << MapDesign::UnchangedColor;   // Do not change color

    newBranchHeadingColors << QColor(Qt::green);
    relinkedBranchHeadingColors << QColor(Qt::red);   // FIXME-1 currently unused

    // Should links of branches use a default color or the color of heading?
    linkColorHintInt = LinkObj::DefaultColor;
    defaultLinkCol = Qt::blue;

    linkStyles << LinkObj::NoLink;
    linkStyles << LinkObj::Parabel;
    //linkStyles << LinkObj::PolyLine;
    //linkStyles << LinkObj::Line;
    //linkStyles << LinkObj::PolyParabel;
    //linkStyles << LinkObj::Parabel;
}

void MapDesign::setName(const QString &s)
{
    name = s;
}

QString MapDesign::getName()
{
    return name;
}

Container::Layout MapDesign::branchesContainerLayout(
        const BranchContainer::StyleUpdateMode &mode,
        int depth)
{
    //qDebug() << "MD  mode=" << mode << " d=" << depth;
    if (mode == BranchContainer::NewBranch) {   // FIXME-0 doesn't look like "relinked", see next line
        // Relinked branch
        int max = bcNewBranchLayouts.count();
        if (depth < max)
            return bcNewBranchLayouts.at(depth);
        else {
            if (max > 0)
                // Return last entry, if it exists
                return bcNewBranchLayouts.at(max - 1);
            else
                // Don't change 
                return Container::UndefinedLayout;
        }
    } else {
        // New branch or anyway updating 
        int max = bcRelinkBranchLayouts.count();
        if (depth < max)
            return bcRelinkBranchLayouts.at(depth);
        else {
            if (max > 0)
                // Return last entry, if it exists
                return bcRelinkBranchLayouts.at(max - 1);
            else
                // Don't change 
                return Container::UndefinedLayout;
        }
    }
}

Container::Layout MapDesign::imagesContainerLayout(
        const BranchContainer::StyleUpdateMode &mode,
        int depth)
{
    //qDebug() << "MD  mode=" << mode << " d=" << depth;
    if (mode == BranchContainer::NewBranch) {
        // Relinked branch
        int max = icNewBranchLayouts.count();
        if (depth < max)
            return icNewBranchLayouts.at(depth);
        else {
            if (max > 0)
                // Return last entry, if it exists
                return icNewBranchLayouts.at(max - 1);
            else
                // Don't change 
                return Container::UndefinedLayout;
        }
    } else {
        // New branch or anyway updating 
        int max = icRelinkBranchLayouts.count();
        if (depth < max)
            return icRelinkBranchLayouts.at(depth);
        else {
            if (max > 0)
                // Return last entry, if it exists
                return icRelinkBranchLayouts.at(max - 1);
            else
                // Don't change 
                return Container::UndefinedLayout;
        }
    }
}

LinkObj::ColorHint MapDesign::linkColorHint()
{
    return linkColorHintInt;
}

void MapDesign::setLinkColorHint(const LinkObj::ColorHint &lch)
{
    linkColorHintInt = lch;
}

QColor MapDesign::defaultLinkColor()
{
    return defaultLinkCol;
}

void MapDesign::setDefaultLinkColor(const QColor &col)
{
    defaultLinkCol = col;
}

LinkObj::Style MapDesign::linkStyle(int depth)
{
    // Special case for now:    // FIXME-3
    // For style PolyParabel or PolyLine in d=1
    // return Parabel or Line for d>1

    if (depth < 2)
        return linkStyles.at(depth);

    if (linkStyles.at(1) == LinkObj::PolyParabel)
        return LinkObj::Parabel;

    if (linkStyles.at(1) == LinkObj::PolyLine)
        return LinkObj::Line;

    return linkStyles.at(1);    // Return eithe Line or Parabel
}

bool MapDesign::setLinkStyle(const LinkObj::Style &style, int depth)
{
    // Special case for now:    // FIXME-3
    // Only set style for d=1
    // (Used to be map-wide setting before 3.x)

    if (linkStyles.count() < 2)
        linkStyles << style;
    else
        linkStyles[1] = style;

    return true;
}

void MapDesign::updateBranchHeadingColor(
        BranchItem *branchItem,
        const BranchContainer::StyleUpdateMode &mode,
        int depth)
{
    if (branchItem) {
        HeadingColorHint colHint;
        if (mode == BranchContainer::NewBranch) {
            int max = newBranchHeadingColorHints.count();
            if (depth < max)
                colHint = newBranchHeadingColorHints.at(depth);
            else {
                if (max > 0)
                    // Return last entry, if it exists
                    colHint = newBranchHeadingColorHints.at(max - 1);
                else {
                    colHint = UndefinedColor;
                }
            }
        } else {
            // RelinkedBranch
            int max = relinkedBranchHeadingColorHints.count();
            if (depth < max)
                colHint = relinkedBranchHeadingColorHints.at(depth);
            else {
                if (max > 0)
                    // Return last entry, if it exists
                    colHint = relinkedBranchHeadingColorHints.at(max - 1);
                else {
                    colHint = UndefinedColor;
                }
            }
        }
        QColor col;
        switch (colHint) {
            case InheritedColor: {
                BranchItem *pi = branchItem->parentBranch();
                if (pi) {
                    col = pi->getHeadingColor();
                    break;
                }

            }
                // If there is no parent branch, mapCenter should 
                // have a specific color, thus continue
            case SpecificColor:
                if (mode == BranchContainer::NewBranch) {
                    int max = newBranchHeadingColors.count();
                    if (depth < max)
                        col = newBranchHeadingColors.at(depth);
                    else {
                        if (max > 0)
                            // Return last entry, if it exists
                            col = newBranchHeadingColors.at(max - 1);
                        else {
                            qWarning() << "No specific color found for new branch";
                            col = QColor("#EEEEEE");
                        }
                    }
                } else {
                    int max = relinkedBranchHeadingColors.count();
                    if (depth < max)
                        col = relinkedBranchHeadingColors.at(depth);
                    else {
                        if (max > 0)
                            // Return last entry, if it exists
                            col = relinkedBranchHeadingColors.at(max - 1);
                        else {
                            qWarning() << "No specific color found for relinked branch";
                            col = QColor("#EEEEEE");
                        }
                    }
                }
                branchItem->setHeadingColor(col);
                break;
            case UnchangedColor:
                return;
            default:
                qWarning() << "MapDesign::updateBranchHeadingColor no newBranchHeadingColorHint defined";
        }
        branchItem->setHeadingColor(col);
    }
}

QColor MapDesign::selectionColor()
{
    return selectionColorInt;
}

void MapDesign::setSelectionColor(const QColor &c)
{
    selectionColorInt = c;
}
