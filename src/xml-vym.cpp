#include "xml-vym.h"

#include <QColor>
#include <QMessageBox>
#include <QTextStream>
#include <typeinfo>

#include "attributeitem.h"
#include "branchitem.h"
#include "flag.h"
#include "mainwindow.h"
#include "misc.h"
#include "settings.h"
#include "slideitem.h"
#include "task.h"
#include "taskmodel.h"
#include "xlinkitem.h"
#include "xlinkobj.h"

extern Main *mainWindow;
extern Settings settings;
extern TaskModel *taskModel;
extern QString vymVersion;

VymReader::VymReader(VymModel* m)
    : BaseReader(m)
{
    //qDebug() << "Constr. VymReader";
}

bool VymReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("vymmap")) {
            readVymMap();
        } else {
            xml.raiseError("No vymmap as next element.");
        }
    }

    return !xml.error();
}


void  VymReader::raiseUnknownElementError()
{
    xml.raiseError("Found unknown element: " + xml.name().toString());
}

void VymReader::readVymMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("vymmap"));

    // Check version
    if (!xml.attributes().hasAttribute("version")) {
        xml.raiseError("No version found for vymmap.");
        return;
    }

    // FIXME-2 move reading of version attribute to readVymMapAttr()
    if (!xml.attributes().value("version").isEmpty()) {
        version = xml.attributes().value("version").toString();
        if (!versionLowerOrEqualThanVym(version)) {
            QMessageBox::warning(
                0, QObject::tr("Warning: Version Problem"),
                QObject::tr(
                    "<h3>Map is newer than VYM</h3>"
                    "<p>The map you are just trying to load was "
                    "saved using vym %1. "
                    "The version of this vym is %2. "
                    "If you run into problems after pressing "
                    "the ok-button below, updating vym should help.</p>")
                    .arg(version)
                    .arg(vymVersion));
        }
        else
            model->setVersion(version); // FIXME-1 really needed? what for?
    }

    branchesTotal = 0;
    branchesCounter = 0;

    if (loadMode == File::NewMap || loadMode == File::DefaultMap) {
        // Create mapCenter
        model->clear();
        lastBranch = nullptr;

        readVymMapAttr();
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("mapcenter"))
            readMapCenter();
        else if (xml.name() == QLatin1String("select"))
            readSelect();
        else {
            raiseUnknownElementError();
            return;
        }
    }
}

void VymReader::readSelect()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("select"));

    QString s = xml.readElementText();
    model->select(s);
}

void VymReader::readMapCenter()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("mapcenter"));

    if (loadMode == File::NewMap) {
        // Really use this as mapCenter in a new map
        lastBranch = model->createMapCenter();
    } else {
        // Use the "mapcenter" from xml  as a "branch"
        // in an existing map
        BranchItem *bi = model->getSelectedBranch();
        if (bi) {
            lastBranch = bi;
            if (loadMode == File::ImportAdd) {
                // Import Add
                if (insertPos < 0)
                    lastBranch = model->createBranch(lastBranch);
                else {
                    lastBranch = model->addNewBranch(lastBranch, insertPos);
                    insertPos++;
                }
            }
            else {
                // Import Replace
                if (insertPos < 0) {
                    insertPos = lastBranch->num() + 1;
                    model->clearItem(lastBranch);
                }
                else {
                    BranchItem *pi = bi->parentBranch();
                    lastBranch = model->addNewBranch(pi, insertPos);
                    insertPos++;
                }
            }
        } else
            // if nothing selected, add mapCenter without parent
            lastBranch = model->createMapCenter();
    }
    readBranchAttr();

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("heading"))
            readHeading();
        else if (xml.name() == QLatin1String("branch"))
            readBranch();
        else if (xml.name() == QLatin1String("frame"))
            readFrame();
        else {
            raiseUnknownElementError();
            return;
        }
    }

    model->emitDataChanged(lastBranch);
}

void VymReader::readBranch()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("branch"));

    lastBranch = model->createBranch(lastBranch);
    readBranchAttr();

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("heading"))
            readHeading();
        else if (xml.name() == QLatin1String("branch"))
            readBranch();
        else if (xml.name() == QLatin1String("frame"))
            readFrame();
        else {
            raiseUnknownElementError();
            return;
        }
    // FIXME-00 cont here with frame and images
    }

    // FIXME-0 checkMoreElements();

    // Empty branches may not be scrolled
    // (happens if bookmarks are imported)
    if (lastBranch->isScrolled() && lastBranch->branchCount() == 0)
        lastBranch->unScroll();

    // FIXME-0 needed? lastBranch->getBranchContainer()->updateStyles(BranchContainer::RelinkBranch);
    model->emitDataChanged(lastBranch);

    lastBranch = (BranchItem *)(lastBranch->parent());
    lastBranch->setLastSelectedBranch(0);
}

void VymReader::readHeading()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("heading"));

    if (!lastBranch) {
            xml.raiseError("No lastBranch available to set heading.");
            return;
    }

    htmldata.clear();
    vymtext.clear();

    QString a = "fonthint";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        vymtext.setFontHint(s);

    a = "textMode";
    s = xml.attributes().value(a).toString();
    if (s == "richText")
        vymtext.setRichText(true);
    else
        vymtext.setRichText(false);

    a = "textColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        // For compatibility with <= 2.4.0 set both branch and
        // heading color
        QColor col(s);
        lastBranch->setHeadingColor(col);
        vymtext.setColor(col);
    }

    a = "text";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        vymtext.setText(unquoteQuotes(s));

        if (versionLowerOrEqual(version, "2.4.99") &&
            htmldata.contains("<html>"))
            // versions before 2.5.0 didn't use CDATA to save richtext
            vymtext.setAutoText(htmldata);
        else {
            // Versions 2.5.0 to 2.7.562  had HTML data encoded as CDATA
            // Later versions use the <vymnote  text="...">  attribute,
            // which is set already in begin element
            // If both htmldata and vymtext are already available, use the
            // vymtext
            if (vymtext.isEmpty())
                vymtext.setText(htmldata);
        }
        lastBranch->setHeading(vymtext);
    }

    if (xml.readNextStartElement())
        raiseUnknownElementError();
}

void VymReader::readFrame()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("frame"));

    readFrameAttr();

    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
}

void VymReader::readVymMapAttr()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("vymmap"));

    QString a = "author";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setAuthor(s);

    a = "title";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setTitle(s);

    a = "comment";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setComment(unquoteMeta(s));

    a = "branchCount";
    s = xml.attributes().value(a).toString();
    bool ok;
    int i = s.toInt(&ok);
    if (!ok) {
        xml.raiseError("Could not parse attribute " + a);
        return;
    }
    branchesTotal = i;
    if (branchesTotal > 10) {
        useProgress = true;
        mainWindow->setProgressMaximum(branchesTotal);
    }

    QColor col;
    a = "backgroundColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col.setNamedColor(s);
        model->getScene()->setBackgroundBrush(col);
    }

    a = "defaultFont";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        QFont font;
        font.fromString(s);
        model->setMapDefaultFont(font);
    }

    a = "selectionColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col.setNamedColor(s);
        model->setSelectionColor(col);
    }

    a = "linkColorHint";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        if (s == "HeadingColor")
            model->setLinkColorHint(LinkObj::HeadingColor);
        else
            model->setLinkColorHint(LinkObj::DefaultColor);
    }

    a = "linkStyle";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setMapLinkStyle(s);

    a = "linkColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col.setNamedColor(s);
        model->setDefaultLinkColor(col);
    }

    QPen pen(model->getMapDefXLinkPen());
    a = "defXLinkColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col.setNamedColor(s);
        pen.setColor(col);
    }

    a = "defXLinkWidth";
    s = xml.attributes().value(a).toString();
    i = s.toInt(&ok);
    if (!ok) {
        xml.raiseError("Could not parse attribute  " + a);
        return;
    }
    pen.setWidth(i);

    a = "defXLinkPenStyle";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        bool ok;
        Qt::PenStyle ps = penStyle(s, ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        pen.setStyle(ps);
    }
    model->setMapDefXLinkPen(pen);

    a = "defXLinkStyleBegin";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setMapDefXLinkStyleBegin(s);

    a = "defXLinkStyleEnd";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setMapDefXLinkStyleEnd(s);

    a = "mapZoomFactor";
    s = xml.attributes().value(a).toString();
    qreal r = s.toDouble(&ok);
    if (!ok) {
        xml.raiseError("Could not parse attribute" + a);
        return;
    }
    model->setMapZoomFactor(r);

    a = "mapRotationAngle";
    s = xml.attributes().value(a).toString();
    r = s.toDouble(&ok);
    if (!ok) {
        xml.raiseError("Could not parse attribute " + a);
        return;
    }
    model->setMapRotationAngle(r);
}

void VymReader::readBranchAttr()
{
    Q_ASSERT(xml.isStartElement() && (
            xml.name() == QLatin1String("branch") ||
            xml.name() == QLatin1String("mapcenter")));

    branchesCounter++;
    if (useProgress)
        mainWindow->addProgressValue((float)branchesCounter / branchesTotal);

    lastMI = lastBranch;
    BranchContainer *lastBC = lastBranch->getBranchContainer();

    readOrnamentsAttr();

    QString a = "scrolled";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        lastBranch->toggleScroll();

    a = "incImgV";
    s = xml.attributes().value(a).toString();
    if (s == "true")      // pre 2.9 feature
        lastBranch->setImagesLayout("FloatingBounded");

    a = "incImgH";
    s = xml.attributes().value(a).toString();
    if (s == "true")      // pre 2.9 feature
        lastBranch->setImagesLayout("FloatingBounded");

    a = "childrenFreePos";
    s = xml.attributes().value(a).toString();
    if (s == "true")      // pre 2.9 feature
        lastBranch->setBranchesLayout("FloatingBounded");

    // Container layouts
    a = "branchesLayout";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        lastBranch->setBranchesLayout(s);
        lastBC->branchesContainerAutoLayout = false;
    }

    a = "imagesLayout";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        lastBC->imagesContainerAutoLayout = false;
        lastBranch->setImagesLayout(s);
    }

    bool ok;
    qreal r;
    a = "rotHeading";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setRotationHeading(r);
    }

    a = "rotContent";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setRotationSubtree(r);
    }
}

void VymReader::readOrnamentsAttr() // FIXME-0 not ported yet
{
    Q_ASSERT(xml.isStartElement() && (
            xml.name() == QLatin1String("branch") ||
            xml.name() == QLatin1String("mapcenter")));
}

void VymReader::readFrameAttr()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("frame"));

    if (lastBranch) {
        BranchContainer *bc = lastBranch->getBranchContainer();

        bool useInnerFrame = true;
        // useInnerFrame was introduced in 2.9.506
        // It replaces the previous "includeChildren" attribute
        QString a = "includeChildren";
        QString s = xml.attributes().value(a).toString();
        if (s == "true")
            useInnerFrame = false;

        a = "frameType";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty())
            bc->setFrameType(useInnerFrame, s);
        a = "penColor";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty())
            bc->setFramePenColor(useInnerFrame, s);
        a = "brushColor";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            bc->setFrameBrushColor(useInnerFrame, s);
            lastMI->setBackgroundColor(s);
        }

        int i;
        bool ok;
        a = "padding";
        s = xml.attributes().value(a).toString();
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePadding(useInnerFrame, i);

        a = "borderWidth";
        s = xml.attributes().value(a).toString();
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePenWidth(useInnerFrame, i);

        a = "penWidth";
        s = xml.attributes().value(a).toString();
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePenWidth(useInnerFrame, i);
    }
}

