#include "version.h"

#include <QRegExp>

bool checkVersion (const QString &v)
{
    // returns true, if vym is able to read file regarding 
    // the version set with setVersion
    return checkVersion (v,__VYM_VERSION);
}


bool checkVersion (const QString &v, const QString &d)
{
    bool ok;
    int v1;
    int v2;
    int v3;
    int d1;
    int d2;
    int d3;

    QRegExp rx("(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})");
    int pos=rx.indexIn (v);
    if (pos>-1)
    {
	v1=rx.cap(1).toInt(&ok);
	v2=rx.cap(2).toInt(&ok);
	v3=rx.cap(3).toInt(&ok);
    } else
	qWarning (QString ("Warning: Checking version failed for v=%1").arg(v));

    pos=rx.indexIn (d);
    if (pos>-1)
    {
	d1=rx.cap(1).toInt(&ok);
	d2=rx.cap(2).toInt(&ok);
	d3=rx.cap(3).toInt(&ok);
    } else
	qWarning (QString ("Warning: Checking version failed for d=%1").arg(d));

    
    if (d1 > v1)
	return true;
    if (d1 < v1)
	return false;
    if (d2 > v2)
	return true;
    if (d2 < v2)
	return false;
    if (d3 > v3)
	return true;
    if (d3 < v3)
	return false;
    return true;    

}
