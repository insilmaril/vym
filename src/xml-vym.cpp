#include "xml-vym.h"

#include <QColor>
#include <QMessageBox>
#include <QTextStream>

#include "attributeitem.h"
#include "branchitem.h"
#include "flag.h"
#include "mainwindow.h"
#include "misc.h"
#include "settings.h"
#include "slideitem.h"
#include "task.h"
#include "taskmodel.h"
#include "vymmodel.h"
#include "xlink.h"
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

    // When importing maps, content could be filtered,
    // e.g. without slides
    contentFilter = 0x0000;

    branchesTotal = 0;
    useProgress = false;

    lastBranch = nullptr;
    lastMI = nullptr;
}

bool VymReader::read(QIODevice *device)
{
    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("vymmap")) {
            readVymMap();
        } else {
            xml.raiseError("No vymmap or heading as next element.");
        }
    }
    return !xml.error();
}

void VymReader::readVymMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("vymmap"));

    // Check version
    if (!xml.attributes().hasAttribute("version")) {
        xml.raiseError("No version found for vymmap.");
        return;
    }

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
                    .arg(version, vymVersion));
        }
        model->setMapVersion(version);
    }

    branchesTotal = 0;
    branchesCounter = 0;

    if (loadMode == File::NewMap || loadMode == File::DefaultMap) {
        // Create mapCenter
        model->clear();
        lastBranch = model->getRootItem();

        readVymMapAttr();
    } else {
        // Imports need a dedicated branch
        lastBranch = insertBranch;

        if (loadMode == File::ImportReplace) {
            if (!lastBranch) {
                xml.raiseError("readVymMap - Import/Replace map, but nothing selected!");
                return;
            }

            insertPos = lastBranch->num();
            BranchItem *pb = lastBranch->parentBranch();
            if (!pb) {
                xml.raiseError("readVymMap - No parent branch for selection in ImportReplace!");
                return;
            }

            model->deleteItem(lastBranch);
            lastBranch = pb;
            loadMode = File::ImportAdd;
        } else {
            // ImportAdd
            if (insertPos < 0)
                insertPos = 0;
        }
    }

    if (!lastBranch)
        // Make sure, that mapcenters can be pasted on empty map e.g. for undo
        lastBranch = model->getRootItem();

    lastMI = lastBranch;


    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("mapdesign"))
            readMapDesign();
        else if (xml.name() == QLatin1String("mapcenter") ||
            xml.name() == QLatin1String("branch")) {
            readBranchOrMapCenter(loadMode, insertPos);
            insertPos++;
        } else if (xml.name() == QLatin1String("floatimage"))
            readImage();    // Used when pasting image
        else if (xml.name() == QLatin1String("setting"))
            readSetting();
        else if (xml.name() == QLatin1String("select"))
            readSelection();
        else if (xml.name() == QLatin1String("userflagdef"))
            readUserFlagDef();
        else if (xml.name() == QLatin1String("xlink"))
            readXLink();
        else if (xml.name() == QLatin1String("slide"))
            readSlide();
        else {
            raiseUnknownElementError();
            return;
        }
    }
}

void VymReader::readMapDesign()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("mapdesign"));

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("md"))
            readMapDesignElement();
        else {
            raiseUnknownElementError();
            return;
        }
    }
}

void VymReader::readMapDesignElement()  // FIXME-2 parse the elements, either in VymModel or directly (generic) in MapDesign
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("md"));

    readMapDesignCompatibleAttributes();

    bool ok;
    QString k = xml.attributes().value("key").toString();
    QString v = xml.attributes().value("val").toString();
    QString d = xml.attributes().value("d").toString();
    if (!v.isEmpty()) {
        model->mapDesign()->setElement(k, v, d);    // FIXME-2 Check return val for error
    }

    if (xml.readNextStartElement())
        raiseUnknownElementError();
}

void VymReader::readMapDesignCompatibleAttributes()
{
    // Reads attributes which before 2.9.13 used to be
    // in <vymmap> and now are in <mapdesign>

    Q_ASSERT(xml.isStartElement() &&
            ( xml.name() == QLatin1String("vymmap") ||
              xml.name() == QLatin1String("md")));

    QString a = "backgroundColor";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        model->setBackgroundColor(QColor(s));
    }

    a = "backgroundImage";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setBackgroundImage(parseHREF(s));

    a = "backgroundImageName";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        model->setBackgroundImageName(s);
    }

    a = "font";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        QFont font;
        font.fromString(s);
        model->mapDesign()->setFont(font);
    }

    QColor col;

    // Only for backwards compatibility reading <vymmap>.
    // moved to <mapdesign> starting 2.9.513
    a = "selectionColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col = QColor::fromString(s);
        model->setSelectionPenColor(col);
        model->setSelectionBrushColor(col);
    }

    // Only for backwards compatibility reading <vymmap>.
    // moved to <mapdesign> starting 2.9.513
    a = "selectionPenColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        col = QColor::fromString(s);
        model->setSelectionPenColor(col);
    }

    bool ok;
    // Only for backwards compatibility reading <vymmap>.
    // moved to <mapdesign> starting 2.9.513
    a = "selectionPenWidth"; 
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        float  w = s.toFloat(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute  " + a);
            return;
        }
        model->setSelectionPenWidth(w);
    }

    a = "selectionBrushColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setSelectionBrushColor(QColor(s));

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
    if (!s.isEmpty()) {
        // Legacy Pre 2.9.518: Style defined in <vymmap> for all levels
        model->setLinkStyle(s);
    }

    a = "linkColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        model->setDefaultLinkColor(QColor(s));
    }

    QPen pen(model->mapDesign()->defXLinkPen());
    a = "defXLinkColor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        if (!s.isEmpty()) {
            col = QColor::fromString(s);
            pen.setColor(col);
        }
    }

    a = "defXLinkWidth";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        int i = s.toInt(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute  " + a);
            return;
        }
        pen.setWidth(i);
    }

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
    model->setDefXLinkPen(pen);

    a = "defXLinkStyleBegin";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setDefXLinkStyleBegin(s);

    a = "defXLinkStyleEnd";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        model->setDefXLinkStyleEnd(s);
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

    if (xml.tokenType() == QXmlStreamReader::EndElement) return;

    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
}

void VymReader::readAttribute() // FIXME-3 Checking types no longer needed. Check with firefox export/import
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("attribute"));

    QString key = xml.attributes().value("key").toString();
    QString type = xml.attributes().value("type").toString();   // May be empty!
    QVariant val;
    if (lastBranch && !key.isEmpty() && !type.isEmpty()) {
        if (type == "Integer")
            val = xml.attributes().value("value").toInt();
        else if (type == "QString" || type == "String")
            val = xml.attributes().value("value").toString();
        else if (type == "QDateTime" || type == "DateTime")
            val = QDateTime::fromString(
                    xml.attributes().value("value").toString(),
                    Qt::ISODate);
        else if (type == "Undefined") {
            val = xml.attributes().value("value").toString();
            qWarning() << "Found attribute type 'Undefined': " << val;
        } else {
            xml.raiseError("readAttribute: Found unknown attribute type");
            return;
        }

        model->setAttribute(lastBranch, key, val);
    }

    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
}

void VymReader::readBranchOrMapCenter(File::LoadMode loadModeBranch, int insertPosBranch)
{
    Q_ASSERT(xml.isStartElement() &&
            (xml.name() == QLatin1String("branch") ||
             xml.name() == QLatin1String("mapcenter")));

    // Create branch or mapCenter
    if (loadModeBranch == File::NewMap || loadModeBranch == File::DefaultMap)
        lastBranch = model->createBranchWhileLoading(lastBranch);
    else {
        // For Imports create branch at insertPos
        // (Here we only use ImportInsert, replacements already have
        // been done before)
        if (loadModeBranch == File::ImportAdd)
            lastBranch = model->createBranchWhileLoading(lastBranch, insertPos);
    }
    // Prepare parsing heading later
    lastMI = lastBranch;

    readBranchAttr();

    // While going deeper, no longer "import" but just load as usual
    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("heading") ||
            xml.name() == QLatin1String("vymnote") ||
            xml.name() == QLatin1String("htmlnote") ||
            xml.name() == QLatin1String("note"))
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
        else if (xml.name() == QLatin1String("task"))
            readTask();
        else if (xml.name() == QLatin1String("floatimage"))
            readImage();
        else if (xml.name() == QLatin1String("attribute"))
            readAttribute();
        else if (xml.name() == QLatin1String("xlink"))
            readLegacyXLink();
        else {
            raiseUnknownElementError();
            return;
        }
    }

    // Empty branches may not be scrolled
    // (happens if bookmarks are imported)
    if (lastBranch->isScrolled() && lastBranch->branchCount() == 0)
        lastBranch->unScroll();

    lastBranch->updateVisuals();


    lastBranch = lastBranch->parentBranch();
    lastBranch->setLastSelectedBranch(0);
}

void VymReader::readHeadingOrVymNote()
{
    Q_ASSERT(xml.isStartElement() &&
            (xml.name() == QLatin1String("heading") ||
             xml.name() == QLatin1String("vymnote") ||
             xml.name() == QLatin1String("htmlnote") ||
             xml.name() == QLatin1String("note") ));

    if (!lastMI) {
        xml.raiseError("No lastMI available to set <heading>, <vymnote>, or <htmlnote>.");
        return;
    }

    // Save type for later (after reading html)
    QString textType = xml.name().toString();
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

    QString href = xml.attributes().value("href").toString();
    a = "text";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        vymtext.setText(unquoteQuotes(s));
    } else if (!href.isEmpty()) {
        // <note> element using an external file with href="..."
        // only for backward compatibility (<1.4.6).
        // Later htmlnote was used and meanwhile vymnote.
        QString fn = parseHREF(href);
        QFile file(fn);

        if (!file.open(QIODevice::ReadOnly)) {
            xml.raiseError("parseVYMHandler::readLegacyNote:  Couldn't load " + fn);
            return;
        }
        QTextStream stream(&file);
        QString lines;
        while (!stream.atEnd()) {
            lines += stream.readLine() + "\n";
        }
        file.close();

        if (lines.contains("<html"))
            vymtext.setRichText(lines);
        else
            vymtext.setPlainText(lines);

        xml.readNext();
        if (xml.tokenType() == QXmlStreamReader::Characters) {
            htmldata += xml.text().toString();
            qWarning() << "Found characters AND href in legacy <note> element. Ignoring characters...";
            // Read to end element. There should be no <html> coming up...
            xml.readNext();
            if (xml.tokenType() != QXmlStreamReader::EndElement) {
                xml.raiseError(QString("Found unexpected element <%1>").arg(xml.name()));
                return;
            }
        }
    } else {
        // Legacy versions did not use the "text" attribute, 
        // but had the content as characters or inline <html>

        bool finished = false;
        while (!finished) {
            xml.readNext();
            if (xml.tokenType() == 1) {
                xml.raiseError(QString("Invalid token  found: " +  xml.errorString()));
                return;
            }
            switch(xml.tokenType())
            {
                case QXmlStreamReader::StartElement:
                    if (xml.name() == QLatin1String("html")) {
                        vymtext.setRichText(true);
                        readHtml();
                    } else {
                        raiseUnknownElementError();
                        return;
                    }
                    break;
                case QXmlStreamReader::EndElement:
                    if (xml.name().toString() != textType) {
                        xml.raiseError("Expected end token: " + textType + " but found " + xml.name().toString());
                        return;
                    }
                    finished = true;
                    break;
                case QXmlStreamReader::Characters:
                    htmldata += xml.text().toString();
                    break;
                default:
                    break;
            }
        }
    } // Legacy text as characters instead of text attribute

    //qDebug() << "xml.name()=" <<xml.name() << " " << xml.tokenString()<<" htmldata: " << htmldata << " vT=" <<vymtext.getText();

    if (versionLowerOrEqual(version, "2.4.99") &&
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

    if (textType == "heading") {
        a = "textColor";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            QColor col(s);
            vymtext.setColor(col);

            // For compatibility with <= 2.4.0 set both branch and
            // heading color
            lastMI->setHeadingColor(col);
        }

        lastMI->setHeading(vymtext);
    } else {
        if (lastMI->hasTypeBranch()) {
            if (textType == "vymnote" || textType == "note" || textType == "htmlnote")
                lastMI->setNote(vymtext);
            else {
                qDebug() << "texttype=" << textType << " in line " << xml.lineNumber() << lastMI->headingText();
                xml.raiseError("Trying to set note for lastMI which is not a branch");
                return;
            }
        }
    }


    if (xml.tokenType() == QXmlStreamReader::EndElement) return;

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

void VymReader::readLegacyXLink()
{ // only for backward compatibility
  // Before 1.13.2 xlinks used to be part of <branch>
    Q_ASSERT(xml.isStartElement() &&
            xml.name() == QLatin1String("xlink"));

    QString a = "beginID";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        TreeItem *beginBI = model->findBySelectString(s);
        a = "endID";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            TreeItem *endBI = model->findBySelectString(s);
            if (beginBI && endBI && beginBI->hasTypeBranch() && endBI->hasTypeBranch()) {
                XLink *xl = new XLink(model);
                xl->setBeginBranch((BranchItem *)beginBI);
                xl->setEndBranch((BranchItem *)endBI);
                model->createXLink(xl);

                QPen pen = xl->getPen();

                a = "color";
                s = xml.attributes().value(a).toString();
                if (!s.isEmpty()) {
                    QColor col;
                    col = QColor::fromString(s);
                    pen.setColor(col);
                }

                a = "width";
                s = xml.attributes().value(a).toString();
                if (!s.isEmpty()) {
                    bool okx;
                    pen.setWidth(s.toInt(&okx, 10));
                }
                xl->setPen(pen);
            }
        }
    }

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

    if (xml.readNextStartElement()) {
        raiseUnknownElementError();
        return;
    }
}

void VymReader::readImage()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("floatimage"));

    lastImage = model->createImage(lastBranch);
    lastMI = lastImage;

    QString s;

    s = attributeToString("href");
    if (!s.isEmpty()) {
        // Load Image
        if (!lastImage->load(parseHREF(s))) {
            QMessageBox::warning(0, "Warning: ",
                                 "Couldn't load image\n" +
                                     parseHREF(s));
            lastImage = nullptr;
            return;
        }
    }

    // Scale image
    // scaleX and scaleY are no longer used since 2.7.509 and replaced by
    // scaleFactor
    float x = 1;
    float y = 1;
    bool okx, oky;
    s = attributeToString("scaleX");
    if (!s.isEmpty()) {
        x = s.toFloat(&okx);
        if (!okx) {
            xml.raiseError("Couldn't read scaleX of image");
            return;
        }
    }

    s = attributeToString("scaleY");
    if (!s.isEmpty()) {
        y = s.toFloat(&oky);
        if (!oky) {
            xml.raiseError("Couldn't read scaleY of image");
            return;
        }
    }

    s = attributeToString("scale");
    if (!s.isEmpty()) {
        x = s.toFloat(&okx);
        if (!okx) {
            xml.raiseError("Couldn't read scale of image");
            return;
        }
    }

    s = attributeToString("scaleFactor"); // Legacy: Used in version < 2.9.518
    if (!s.isEmpty()) {
        x = s.toFloat(&okx);
        if (!okx) {
            xml.raiseError("Couldn't read scaleFactor of image");
            return;
        }
    }

    if (x != 1)
        lastImage->setScale(x);

    readOrnamentsAttr();

    s = attributeToString("originalName");
    if (!s.isEmpty())
        lastImage->setOriginalFilename(s);

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("heading"))
            readHeadingOrVymNote();
        else {
            raiseUnknownElementError();
            return;
        }
    }
}

void VymReader::readXLink()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("xlink"));

    QString s;

    QString beginID = xml.attributes().value("beginID").toString();
    QString endID = xml.attributes().value("endID").toString();
    TreeItem *beginBI;
    TreeItem *endBI;

    if (beginID.contains(":")) {
        // Legacy versions <= 2.9.533
        beginBI = model->findBySelectString(beginID);
        endBI = model->findBySelectString(endID);
    } else {
        // Version >= 2.9.534
        beginBI = model->findUuid(QUuid(beginID));
        endBI = model->findUuid(QUuid(endID));
    }

    if (beginBI && endBI && beginBI->hasTypeBranch() && endBI->hasTypeBranch()) {
        XLink *xl = new XLink(model);
        xl->setBeginBranch((BranchItem *)beginBI);
        xl->setEndBranch((BranchItem *)endBI);

        model->createXLink(xl);

        QPen pen = xl->getPen();
        bool ok;
        s = attributeToString("color");
        if (!s.isEmpty())
            pen.setColor(QColor(s));

        s = attributeToString("type");
        if (!s.isEmpty())
            xl->setLinkType(s);

        s = attributeToString("width");
        if (!s.isEmpty())
            pen.setWidth(s.toInt(&ok, 10));

        s = attributeToString("penstyle");
        if (!s.isEmpty())
            pen.setStyle(penStyle(s, ok));

        xl->setPen(pen);

        s = attributeToString("styleBegin");
        if (!s.isEmpty())
            xl->setStyleBegin(s);

        s = attributeToString("styleEnd");
        if (!s.isEmpty())
            xl->setStyleEnd(s);

        /* FIXME-3 better set control points via VymModel for saveState
         * (no longer include XLO then...)
        */

        XLinkObj *xlo = xl->getXLinkObj();
        s = attributeToString("c0");
        if (xlo && !s.isEmpty()) {
            QPointF p = point(s, ok);
            if (ok)
                xlo->setC0(p);
        }
        s = attributeToString("c1");
        if (xlo && !s.isEmpty()) {
            QPointF p = point(s, ok);
            if (ok)
                xlo->setC1(p);
        }
        s = attributeToString("uuid");
        if (!s.isEmpty())
            xl->setUuid(s);

    }

    if (xml.readNextStartElement())
        raiseUnknownElementError();
}

void VymReader::readSlide()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("slide"));

    QStringList scriptlines; // FIXME-3 needed for switching to inScript
                             // Most attributes are obsolete with inScript

    QString s;
    bool ok;
    qreal r;
    int i;

    if (!(contentFilter & SlideContent)) {
        lastSlide = model->addSlide();

        s = attributeToString("name");
        if (!s.isEmpty())
            lastSlide->setName(s);

        s = attributeToString("zoom");
        if (!s.isEmpty()) {
            r = s.toDouble(&ok);
            if (ok) scriptlines.append(QString("setZoom(%1)").arg(r));
        }

        s = attributeToString("rotation");
        if (!s.isEmpty()) {
            r = s.toDouble(&ok);
            if (ok) scriptlines.append(QString("setRotation(%1)").arg(r));
        }

        s = attributeToString("duration");
        if (!s.isEmpty()) {
            i = s.toInt(&ok);
            if (ok) scriptlines.append(QString("setAnimCurve(%1)").arg(i));
        }

        s = attributeToString("curve");
        if (!s.isEmpty()) {
            i = s.toInt(&ok);
            if (ok) scriptlines.append(QString("setAnimDuration(%1)").arg(i));
        }

        s = attributeToString("mapitem");
        if (!s.isEmpty()) {
            TreeItem *ti = model->findUuid(QUuid(s));
            if (ti) scriptlines.append(
                QString("centerOnID(\"%1\")").arg(ti->getUuid().toString()));
        }

        // Up to 2.9.0 at least only inScript seems to be used
        s = attributeToString("inScript");
        if (!s.isEmpty())
            lastSlide->setInScript(unquoteMeta(s));
        else
            lastSlide->setInScript(unquoteMeta(scriptlines.join(";\n"))); // FIXME-3 unquote needed? Not used currently anyway

        s = attributeToString("outScript");
        if (!s.isEmpty())
            lastSlide->setOutScript(unquoteMeta(s));
    }

    if (xml.readNextStartElement())
        raiseUnknownElementError();
}

void VymReader::readTask()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("task"));

    if (lastBranch) {
        lastTask = taskModel->createTask(lastBranch);

        QString s = attributeToString("status");
        if (!s.isEmpty())
            lastTask->setStatus(s);

        s = attributeToString("awake");
        if (!s.isEmpty())
            lastTask->setAwake(s);

        s = attributeToString("date_creation");
        if (!s.isEmpty())
            lastTask->setDateCreation(s);

        s = attributeToString("date_modification");
        if (!s.isEmpty())
            lastTask->setDateModification(s);

        s = attributeToString("date_sleep");
        if (!s.isEmpty()) {
            if (!lastTask->setDateSleep(s)) {
                xml.raiseError("Could not set sleep time for task: " + s);
                return;
            }
        }
        s = attributeToString("prio_delta");
        if (!s.isEmpty()) {
            bool ok;
            int d = s.toInt(&ok);
            if (ok)
                lastTask->setPriorityDelta(d);
        }
    }

    if (xml.readNextStartElement())
        raiseUnknownElementError();
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
    int i;
    bool ok;
    if (!s.isEmpty()) {
        i = s.toInt(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        branchesTotal = i;
    }
    if (branchesTotal > 10) {
        useProgress = true;
        mainWindow->setProgressMaximum(branchesTotal);
    }

    qreal r;
    a = "mapZoomFactor";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute" + a);
            return;
        }
        model->setMapZoomFactor(r);
    }

    a = "mapRotation";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        model->setMapRotation(r);
    }

    readMapDesignCompatibleAttributes();
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
        lastBC->setRotationsAutoDesign(false, false);
        lastBC->setRotationHeading(r);
    }

    a = "rotSubtree";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setRotationsAutoDesign(false, false);
        lastBC->setRotationSubtree(r);
    }

    a = "scaleHeading";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setScaleAutoDesign(false, false);
        lastBC->setScaleHeading(r);
    }

    a = "scaleSubtree";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        r = s.toDouble(&ok);
        if (!ok) {
            xml.raiseError("Could not parse attribute " + a);
            return;
        }
        lastBC->setScaleAutoDesign(false, false);
        lastBC->setScaleSubtree(r);
    }
}

void VymReader::readOrnamentsAttr()
{
    Q_ASSERT(xml.isStartElement() && (
            xml.name() == QLatin1String("branch") ||
            xml.name() == QLatin1String("mapcenter") ||
            xml.name() == QLatin1String("floatimage")));

    float x, y;
    bool okx, oky;

    QString s = attributeToString("posX");
    QString t = attributeToString("posY");
    if (!s.isEmpty() || !t.isEmpty()) {
        x = s.toFloat(&okx);
        y = t.toFloat(&oky);
        if (okx && oky)
            lastMI->setPos(QPointF(x, y));
        else {
            xml.raiseError("Couldn't read position of item");
            return;
        }
    }

    // Only left for compatibility with versions < 2.9.500
    s = attributeToString("relPosX");
    t = attributeToString("relPosY");
    if (!s.isEmpty() || !t.isEmpty()) {
        x = s.toFloat(&okx);
        y = t.toFloat(&oky);
        if (okx && oky)
            lastMI->setPos(QPointF(x, y));
        else {
            xml.raiseError("Couldn't read relative position of item");
            return;
        }
    }

    // Only left for compatibility with versions < 2.9.500
    s = attributeToString("absPosX");
    t = attributeToString("absPosY");
    if (!s.isEmpty() || !t.isEmpty()) {
        x = s.toFloat(&okx);
        y = t.toFloat(&oky);
        if (okx && oky)
            lastMI->setPos(QPointF(x, y));
        else {
            xml.raiseError("Couldn't read absolute position of item");
            return;
        }
    }

    s = attributeToString("url");
    if (!s.isEmpty())
        lastMI->setUrl(s);
    s = attributeToString("vymLink");
    if (!s.isEmpty())
        lastMI->setVymLink(s);
    s = attributeToString("hideInExport");
    if (!s.isEmpty())
        if (s == "true")
            lastMI->setHideInExport(true);

    s = attributeToString("hideLink");
    if (!s.isEmpty()) {
        if (s == "true")
            lastMI->setHideLinkUnselected(true);
        else
            lastMI->setHideLinkUnselected(false);
    }

    s = attributeToString("localTarget");
    if (!s.isEmpty())
        if (s == "true")
            lastMI->toggleTarget();

    s = attributeToString("uuid");
    if (!s.isEmpty()) {
        // While pasting, check for existing UUID
        if (loadMode == File::ImportAdd || loadMode == File::ImportReplace) {
            bool x = model->findUuid(QUuid(s));
            if (!x)
                // Only set Uuid if not adding replacing in map - then duplicate Uuids might cause problems
                // In testing one map will import itself - no new Uuids then.
                lastMI->setUuid(s);
        } else
            lastMI->setUuid(s);
    }

    s = attributeToString("colWidth");
    if (!s.isEmpty()) {
        int i = s.toInt(&okx);
        if (okx) {
            lastBranch->getBranchContainer()->setColumnWidthAutoDesign(false);
            lastBranch->getBranchContainer()->setColumnWidth(i);
        } else {
            xml.raiseError("Couldn't read colWidth of branch");
            return;
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
        bool ok = false;
        QString a = "includeChildren";
        QString s = attributeToString(a);
        if (!s.isEmpty()) {
            ok = true;
            if (s == "true")
                useInnerFrame = false;
        }

        a = "frameUsage";
        s = attributeToString(a);
        if (s == "innerFrame") {
            useInnerFrame = true;
            ok = true;
        } else if (s == "outerFrame") {
            useInnerFrame = false;
            ok =true;
        }

        if (!ok) {
            // In pre 3.0.0 versions "includeChildren" was optional for frame.
            // Without "includeCHildren" and without the later "frameUsage, assume
            // a frame is an innerFrame:
            useInnerFrame = true;
        }

        a = "frameType";
        s = attributeToString(a);
        if (s.isEmpty())
            s = "Rectangle";

        // We will override AutoDesign
        bc->setFrameAutoDesign(useInnerFrame, false);

        // Start with setting/creating frame. 
        // assuming that there is no "NoFrame" frame in the xml
        bc->setFrameType(useInnerFrame, s);

        a = "penColor";
        s = attributeToString(a);
        if (!s.isEmpty())
            bc->setFramePenColor(useInnerFrame, s);

        a = "brushColor";
        s = attributeToString(a);
        if (!s.isEmpty())
            bc->setFrameBrushColor(useInnerFrame, s);

        int i;
        a = "padding";
        s = attributeToString(a);
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePadding(useInnerFrame, i);

        a = "borderWidth";
        s = attributeToString(a);
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePenWidth(useInnerFrame, i);

        a = "penWidth";
        s = attributeToString(a);
        i = s.toInt(&ok);
        if (ok)
            bc->setFramePenWidth(useInnerFrame, i);
    }
}

