#include "xml-freeplane.h"

#define qdbg() qDebug().nospace().noquote()

FreeplaneReader::FreeplaneReader(VymModel* m)
    : BaseReader(m)
{
    qDebug() << "Constr. FreeplaneReader";
}

QString FreeplaneReader::attrString()
{
    QStringList sl;
    for (int i = 0; i < xml.attributes().count(); i++) {
        sl << xml.attributes()[i].name().toString();
    }
    return sl.join(",");
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

    // FIXME-1 implementation missing...
    return !xml.error();
}

void FreeplaneReader::readMap()
{
    Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("map"));

    // Check version// FIXME-2 How to deal with Freeplane versions???
    QString a = "version";
    QString s = xml.attributes().value(a).toString();
    if (s.isEmpty()) {
        xml.raiseError("No version found for Freeplane map.");
        return;
    } else {
        qDebug() << "FR: found version " << s;
    }

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("node")){
            readNode();
        } else {
            raiseUnknownElementError();
            return;
        }
    }
}

void FreeplaneReader::readNode()
{
    QString elementName = "node";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg() << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

    QString a = "TEXT";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        qDebug() << "FP:: Found text=" << s;

    while (xml.readNextStartElement()) {
        if (xml.name() == QLatin1String("icon"))
            readIcon();
        else if (xml.name() == QLatin1String("font"))
            readFont();
        else {
            qdbg() << "Still in node a)";
            raiseUnknownElementError();
            qdbg() << "Still in node a)";
            return;
        }
    }
}

void FreeplaneReader::readIcon()
{
    QString elementName = "icon";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

    QString a = "BUILTIN";
    QString s = xml.attributes().value(a).toString();
    if (!s.isEmpty())
        qDebug() << "FP:: Found builtin icon=" << s;

    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
    xml.readNext();
    qdbg() << "name: " << xml.name() << "  " << xml.tokenString();
}

void FreeplaneReader::readFont()
{
    QString elementName = "font";

    Q_ASSERT(xml.isStartElement() && xml.name() == elementName);

    qdbg () << QString("FP <%1> attributes: %2").arg(elementName).arg(attrString());

}
