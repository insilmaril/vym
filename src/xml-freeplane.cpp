#include "xml-freeplane.h"

#define qdbg() qDebug().nospace().noquote()

#include "branchitem.h"
#include "vymmodel.h"

FreeplaneReader::FreeplaneReader(VymModel* m)
    : BaseReader(m)
{
    qDebug() << "Constr. FreeplaneReader";

    mainBranchRight = nullptr;
    mainBranchLeft = nullptr;
}

void FreeplaneReader::foundElement(const QString &e)
{
    // remember we found this element
    if (!foundElements.contains(e))
        foundElements << e;

    Q_ASSERT(xml.isStartElement() && xml.name() == e);

    // remember its attributes
    QStringList sl = elementAttributes[e];
    for (int i = 0; i < xml.attributes().count(); i++) {
        QString n = xml.attributes()[i].name().toString();
        if (!sl.contains(n))
            sl << n;
    }
    qdbg() << "regElement="  << e << "  sl=" << sl.join(",");
    elementAttributes[e] = sl;
}

QString FreeplaneReader::attrString()
{
    QStringList sl;
    for (int i = 0; i < xml.attributes().count(); i++) {
        sl << xml.attributes()[i].name().toString();
    }
    return sl.join(",");
}

void FreeplaneReader::readToEnd()
{
    QString endName = xml.name().toString();
    while (!xml.isEndElement() || xml.name() != endName) {
        xml.readNext();
        if (xml.isStartElement() && !ignoredElements.contains(xml.name()))
            ignoredElements << xml.name().toString();
    }
}

bool FreeplaneReader::read(QIODevice *device)
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("map"));

    xml.setDevice(device);

    if (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("map")) {
            readMap();
        } else {
            xml.raiseError("No vymmap or heading as next element.");
        }
    }

    // Report what we have found so far
    qdbg() << "Found elements: " << foundElements.join(",");
    foreach (QString e, foundElements) {
        qdbg() << "  Attributes of " << e << "  " << elementAttributes[e];
    }
    qdbg() << "Ignored elements from readToEnd(): " << ignoredElements.join(",");

    // FIXME-3 FreeplaneReader implementation uncomplete: ICON, NOTE, ...
    return !xml.error();
}

void FreeplaneReader::readArrowLink()
{
    QString elementName = "arrowlink";
    foundElement(elementName);

    readToEnd();
}

void FreeplaneReader::readAttribute()
{
    QString elementName = "attribute";
    foundElement(elementName);

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    readToEnd();
}

void FreeplaneReader::readCloud()
{
    QString elementName = "cloud";
    foundElement(elementName);

    readToEnd();
}

void FreeplaneReader::readEdge()
{
    QString elementName = "edge";
    foundElement(elementName);

    readToEnd();
}

void FreeplaneReader::readFont()
{
    QString elementName = "font";
    foundElement(elementName);

    readToEnd();
}

void FreeplaneReader::readIcon()
{
    QString elementName = "icon";
    foundElement(elementName);

    QString a = "BUILTIN";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        qDebug() << "FR:: Found builtin icon=" << s;

    if (xml.tokenType() == QXmlStreamReader::StartElement)
    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
    xml.readNext();
    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
}

void FreeplaneReader::readHook()
{
    QString elementName = "hook";
    foundElement(elementName);

    QString a = "NAME";
    QString s = xml.attributes().value(a).toString();
    if(s == "MapStyle") {
        a = "background";
        s = xml.attributes().value(a).toString();
        if (!s.isEmpty()) {
            QColor col(s);
            model->setBackgroundColor(col);
            qdbg() << "FR::setBackground col = " << s;
        }
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("properties")){
            readProperties();
        } else if (xml.name() == QLatin1String("map_styles")){
            readMapStyles();
        } else if (xml.name() == QLatin1String("Parameters")){
            qdbg () << "FR::readHook  ignoring <Parameters> element...";
            readToEnd();
        } else {
            raiseUnknownElementError();
            return;
        }
    }
}

void FreeplaneReader::readMap()
{
    QString elementName = "map";
    foundElement(elementName);

    // Check version// FIXME-3 How to deal with Freeplane versions???
    QString a = "version";
    QString s = xml.attributes().value(a).toString();
    if (s.isEmpty()) {
        xml.raiseError("No version found for Freeplane map.");
        return;
    } else {
        qDebug() << "FR: found version " << s;
    }

    // Start with clean map 
    model->clear();
    lastBranch = model->getRootItem();

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("node")){
            readNode();
        } else {
            raiseUnknownElementError();
            return;
        }
    }
}

void FreeplaneReader::readMapStyles()
{
    QString elementName = "map_styles";
    foundElement(elementName);

    qdbg() << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());
    readToEnd();
}

void FreeplaneReader::readNode()
{
    QString elementName = "node";
    foundElement(elementName);

    lastBranch = model->createBranchWhileLoading(lastBranch);

    if (lastBranch->depth() == 0) {
        // I am a mapcenter:
        // Create two "helper" branches, because Freeplane seems to haves no
        // relative positioning for mainbranches
        mainBranchLeft = model->createBranchWhileLoading(lastBranch);
        mainBranchRight = model->createBranchWhileLoading(lastBranch);

        mainBranchLeft->setPos(QPointF(-200, 0));
        mainBranchLeft->setHeadingPlainText(" ");

        mainBranchRight->setPos(QPointF(200, 0));
        mainBranchRight->setHeadingPlainText(" ");
    }

    htmldata.clear();
    VymText heading;

    QString a = "TEXT";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        heading.setPlainText(s);

    a = "POSITION";
    s = xml.attributes().value(a).toString();
    if (lastBranch->depth() == 1 && !s.isEmpty()) {
        // Freeplane has a different concept for mainbranches
        // Move either to left or right side of mapcenter
        if (s == "left")
            model->relinkBranch(lastBranch, mainBranchLeft);
        else if (s == "right")
            model->relinkBranch(lastBranch, mainBranchRight);
    }

    a = "FOLDED";
    s = xml.attributes().value(a).toString();
    if (s == "true")
        lastBranch->toggleScroll();

    a = "COLOR";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        lastBranch->setHeadingColor(QColor(s));

    a = "LINK";
    s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        lastBranch->setUrl(s);

    while (xml.readNextStartElement()) {
        qdbg() << "FP::readNode   startElement=" << xml.name();
        if (xml.name() == QLatin1String("icon"))
            readIcon();
        else if (xml.name() == QLatin1String("arrowlink"))
            readArrowLink();
        else if (xml.name() == QLatin1String("attribute"))
            readAttribute();
        else if (xml.name() == QLatin1String("cloud"))
            readCloud();
        else if (xml.name() == QLatin1String("edge"))
            readEdge();
        else if (xml.name() == QLatin1String("font"))
            readFont();
        else if (xml.name() == QLatin1String("hook"))
            readHook();
        else if (xml.name() == QLatin1String("richcontent"))
            readRichContent();
        else if (xml.name() == QLatin1String("node")) {
            readNode();
        } else {
            raiseUnknownElementError();
            return;
        }
    }

    if (!htmldata.isEmpty()) {
        heading.setRichText(htmldata);
        // qdbg() << "htmldata in node: '" <<htmldata << "'";
    }
    lastBranch->setHeading(heading);

    lastBranch->updateVisuals();

    if(lastBranch->depth() == 2) {
        // Hop over helper "mainBranches" back to mapCenter
        lastBranch = lastBranch->parentBranch()->parentBranch();
    } else
        lastBranch = lastBranch->parentBranch();
}

void FreeplaneReader::readProperties()
{
    // Seems to be sub element of <hook  NAME="MAPSTYLE" ...>
    QString elementName = "properties";
    foundElement(elementName);

    QString a = "backgroundImageURI";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty()) {
        model->setBackgroundImage(s);
    }

    readToEnd();
}
void FreeplaneReader::readRichContent()
{
    QString elementName = "richcontent";
    foundElement(elementName);

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("html"))
            readHtml();
        else {
            raiseUnknownElementError();
            return;
        }
    }

    //readToEnd();
}
