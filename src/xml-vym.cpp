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
        } else if (xml.name() == QLatin1String("heading") ||
                   xml.name() == QLatin1String("vymnote"))  { // XML-FIXME-1 test
            // Only read some stuff like VymNote or Heading
            // e.g. for undo/redo
            if (version.isEmpty())
                version = "0.0.0";
            if (!lastBranch) {
                lastBranch = model->getSelectedBranch();
                if (!lastBranch) {
                    xml.raiseError("Found heading element but no branch is selected!");
                    return !xml.error();
                }
            }
            readHeadingOrVymNote();
        } else {
            xml.raiseError("No vymmap or heading as next element.");
        }
    }
    return !xml.error();
}


void  VymReader::raiseUnknownElementError()
{
    xml.raiseError("Found unknown element: " + xml.name().toString());
}

void VymReader::readVymMap() // XML-FIXME-1 test importAdd/importReplace
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
            model->setVersion(version); // XML-FIXME-1 really needed? what for?
    }

    branchesTotal = 0;
    branchesCounter = 0;

    if (loadMode == File::NewMap || loadMode == File::DefaultMap) {
        // Create mapCenter
        model->clear();
        lastBranch = model->getRootItem();

        readVymMapAttr();
    } else {
        // Imports need a selection
        lastBranch = model->getSelectedBranch();
        if (!lastBranch) {
            xml.raiseError("readVymMap - Importing map, but nothing selected!");
            return;
        }

        qDebug() << "a) Importing to lB=" << lastBranch->getHeadingPlain();
        if (loadMode == File::ImportReplace) {
            insertPos = lastBranch->num();
            BranchItem *pb = lastBranch->parentBranch();
            qDebug() << "b) ImportReplace  insPos=" << insertPos << "lastBranch=" << lastBranch << lastBranch->getHeadingPlain();
            if (!pb) {
                xml.raiseError("readVymMap - No parent branch for selection in ImportReplace!");
                return;
            }

            qDebug() << "c) lB deleted: " << insertPos << lastBranch << " new lB=" << pb << pb->getHeadingPlain();
            model->deleteItem(lastBranch);
            lastBranch = pb;
            loadMode = File::ImportAdd;
            qDebug() << "d) loadMode= " << loadMode;
        } else {
            if (insertPos < 0)
                insertPos = 0;
        }
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("mapcenter") ||
            xml.name() == QLatin1String("branch")) {
            readBranchOrMapCenter(loadMode, insertPos);
            insertPos++;
        } else if (xml.name() == QLatin1String("setting"))
            readSetting();
        else if (xml.name() == QLatin1String("select"))
            readSelection();
        else if (xml.name() == QLatin1String("userflagdef"))
            readUserFlagDef();
        else {
            raiseUnknownElementError();
            return;
        }
    }
}

void VymReader::readSelection()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("select"));

    QString s = xml.readElementText();
    model->select(s);
}

void VymReader::readSetting()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("setting"));

    QString k = xml.attributes().value("key").toString();
    if (!k.isEmpty()) {
        QString v = xml.attributes().value("value").toString();
        if (v.isEmpty()) {
            // Version >= 2.5.0 have value as element text
            v = xml.readElementText();
            if (!v.isEmpty()) {
                settings.setLocalValue( model->getDestPath(), k, v);
            }
        } else {
            // Version < 2.5.0 have value as element attribute
            settings.setLocalValue( model->getDestPath(), k, v);
        }
    }
}

void VymReader::readMapCenter() // XML-FIXME-00 merge with readBranchOrMapCenter()?!?
{
    qDebug() << "VM::readMC called!";
}

void VymReader::readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch)
{
    Q_ASSERT(xml.isStartElement() &&
            (xml.name() == QLatin1String("branch") ||
             xml.name() == QLatin1String("mapcenter")));

    qDebug() << "1) readBoMC   lB=" << lastBranch << "loadModeBranch=" << loadModeBranch << "insPos=" << insertPosBranch;

    // Create branch or mapCenter
    if (loadModeBranch == File::NewMap || loadModeBranch == File::DefaultMap) {
        if (lastBranch == model->getRootItem())
            lastBranch = model->createMapCenter();
        else
            lastBranch = model->createBranch(lastBranch);
    } else {
        // For Imports create branch at insertPos
        // (Here we only use ImportInsert, replacements already have
        // been done before)
        if (loadModeBranch == File::ImportAdd) {
            if (insertPosBranch < 0) {
                qDebug () << "  - creating new branch to " << lastBranch->getHeadingPlain();
                lastBranch = model->createBranch(lastBranch);
            }
            else {
                lastBranch = model->addNewBranch(lastBranch, insertPosBranch);
            }
        }
    }
    readBranchAttr();

    // While going deeper, no longer "import" but just load as usual
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("heading") ||
            xml.name() == QLatin1String("vymnote"))
            readHeadingOrVymNote();
        else if (xml.name() == QLatin1String("branch"))
            // Going deeper we regard incoming data as "new", no inserts/replacements
            readBranchOrMapCenter(File::NewMap, -1);
        else if (xml.name() == QLatin1String("frame"))
            readFrame();
        else if (xml.name() == QLatin1String("standardFlag") ||
                 xml.name() == QLatin1String("standardflag"))
            readStandardFlag();
        else if (xml.name() == QLatin1String("userflag"))
            readUserFlag();
    // XML-FIXME-00 cont here with images, notes, ...
        else {
            raiseUnknownElementError();
            return;
        }
    }

    // Empty branches may not be scrolled
    // (happens if bookmarks are imported)
    if (lastBranch->isScrolled() && lastBranch->branchCount() == 0)
        lastBranch->unScroll();

    model->emitDataChanged(lastBranch);

    qDebug() << "  lB before selecting parent: " << lastBranch->getHeadingPlain();
    lastBranch = lastBranch->parentBranch();
    lastBranch->setLastSelectedBranch(0);
}

void VymReader::readHeadingOrVymNote() // XML-FIXME-1 test with legacy vym versions
{
    Q_ASSERT(xml.isStartElement() &&
            (xml.name() == QLatin1String("heading") ||
             xml.name() == QLatin1String("vymnote") ));

    if (!lastBranch) {
            xml.raiseError("No lastBranch available to set heading or vymnote.");
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

    }

    //htmldata += xml.text();  // XML-FIXME-0 test with legacy (at least heading and vymnote. similar note, htmlnote, html)
    htmldata += xml.readElementText(QXmlStreamReader::IncludeChildElements);  // XML-FIXME-0 test with legacy (at least heading and vymnote. similar note, htmlnote, html)
    //qDebug() << "htmldata: " << htmldata << "  xml.name=" << xml.name() << vymtext.getText();
    //qDebug() << xml.tokenType() << xml.tokenString();

    if (versionLowerOrEqual(version, "2.4.99") && // XML-FIXME-1 test with legacy
        htmldata.contains("<html>"))
        // versions before 2.5.0 didn't use CDATA to save richtext
        vymtext.setAutoText(htmldata);
    else {
        // Versions 2.5.0 to 2.7.562  had HTML data encoded as CDATA
        // Later versions use the <heading text="...">  attribute,
        // If both htmldata and vymtext are already available, use the
        // vymtext
        if (vymtext.isEmpty())
            vymtext.setText(htmldata);
    }

    if (xml.name() == "heading")
        lastBranch->setHeading(vymtext);

    if (xml.name() == "vymnote")
        lastBranch->setNote(vymtext);

    if (xml.tokenType() == QXmlStreamReader::EndElement)
        return;

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

void VymReader::readStandardFlag()
{
    Q_ASSERT(xml.isStartElement() &&
            (xml.name() == QLatin1String("standardFlag") ||
             xml.name() == QLatin1String("standardflag")));

    QString s = xml.readElementText();
    lastBranch->activateStandardFlagByName(s);

    /*
    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
    */
}

void VymReader::readUserFlagDef()
{
    Q_ASSERT(xml.isStartElement() &&
             xml.name() == QLatin1String("userflagdef"));

    QString name;
    QString path;
    QString tooltip;
    QUuid uid;

    QString a = "name";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        name = s;

    a = "tooltip";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        tooltip = s;

    a = "uuid";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        uid = QUuid(s);

    Flag *flag;

    a = "href";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        // Setup flag with image
        flag = mainWindow->setupFlag(parseHREF(s), Flag::UserFlag,
                                     name, tooltip, uid);
        if (!flag) {
            xml.raiseError("Couldn't read userflag from: " + s);
            return;
        }
    } else {
        xml.raiseError("readUserFlagDefAttr:  Couldn't read href of flag " + name);
        return;
    }

    a = "group";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        flag->setGroup(s);

    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
}

void VymReader::readUserFlag()
{
    Q_ASSERT(xml.isStartElement() &&
             xml.name() == QLatin1String("userflag"));

    QString a = "uuid";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        lastBranch->toggleFlagByUid(QUuid(s));
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

void VymReader::readOrnamentsAttr()
{
    Q_ASSERT(xml.isStartElement() && (
            xml.name() == QLatin1String("branch") ||
            xml.name() == QLatin1String("mapcenter")));

    if (lastBranch) {
        BranchContainer *bc = lastBranch->getBranchContainer();

        bool okx, oky;
        float x, y;

        QString a = "posX";
        QString s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            x = xml.attributes().value(a).toFloat(&okx);
            a = "posY";
            s = xml.attributes().value(a).toString();
            if (!s.isEmpty()) {
                y = xml.attributes().value(a).toFloat(&oky);
                if (okx && oky)
                    lastMI->setPos(QPointF(x, y));
                else {
                    xml.raiseError("Could not parse attributes posX and posY");
                    return;
                }
            }
        }

        // Only left for compatibility with versions < 2.9.500
        a = "relPosX";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            x = xml.attributes().value(a).toFloat(&okx);
            a = "relPosY";
            s = xml.attributes().value(a).toString();
            if (!s.isEmpty()) {
                y = xml.attributes().value(a).toFloat(&oky);
                if (okx && oky)
                    lastMI->setPos(QPointF(x, y));
                else {
                    xml.raiseError("Could not parse attributes relPosX and relPosY");
                    return;
                }
            }
        }

        // Only left for compatibility with versions < 2.9.500
        a = "absPosX";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            x = xml.attributes().value(a).toFloat(&okx);
            a = "absPosY";
            s = xml.attributes().value(a).toString();
            if (!s.isEmpty()) {
                y = xml.attributes().value(a).toFloat(&oky);
                if (okx && oky)
                    lastMI->setPos(QPointF(x, y));
                else {
                    xml.raiseError("Could not parse attributes absPosX and absPosY");
                    return;
                }
            }
        }
    }
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

