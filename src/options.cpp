#include "options.h"

#include <QApplication>
#include <iostream>

using namespace std;

Option::Option()
{
    name = "";
    sName = "";
    lName = "";
    type = Switch;
    sarg = "";
    active = false;
}

Option::Option(const QString &n, const Type &t, const QString &s,
               const QString &l)
{
    sName = "-" + s;
    lName = "--" + l;
    type = t;
    name = n;
}

void Option::set(const QString &n, const Type &t, const QString &s,
                 const QString &l)
{
    sName = "-" + s;
    lName = "--" + l;
    type = t;
    name = n;
}

QString Option::getName() { return name; }
QString Option::getShort() { return sName; }
QString Option::getLong() { return lName; }
Option::Type Option::getType() { return type; }
void Option::setArg(const QString &s) { sarg = s; }
QString Option::getArg() { return sarg; }
void Option::setActive() { active = true; }
bool Option::isActive() { return active; }

///////////////////////////////////////////////////////////////
Options::Options() {}

int Options::parse()
{
    QStringList arglist = qApp->arguments();

    // Get program name
    progname = arglist.first();
    arglist.pop_front();

    // Work through rest of options
    bool isFile;
    int i = 0;
    for (i = 0; i < arglist.size(); ++i) {
        isFile = true;
        if (arglist[i].left(1) == "-") {
            // Compare given option to all defined options
            for (int j = 0; j < optlist.size(); ++j) {
                if (arglist.at(i) == optlist.value(j).getShort() ||
                    arglist.at(i) == optlist.value(j).getLong()) {
                    optlist[j].setActive();
                    isFile = false;
                    if (optlist[j].getType() == Option::String) {
                        i++;
                        if (i == arglist.size()) {
                            qWarning("Error: argument to option missing");
                            return 1;
                        }
                        optlist[j].setArg(arglist[i]);
                        isFile = false;
                    }
                    break;
                }
            }
            if (isFile) {
                qWarning("Error: Unknown argument ");
                return 1;
            }
        }
        else
            filelist.append(arglist[i]);
    }
    return 0;
}

void Options::add(Option o) { optlist.append(o); }

void Options::add(const QString &n, const Option::Type &t = Option::Switch,
                  const QString &s = "", const QString &l = "")
{
    Option o;
    o.set(n, t, s, l);
    optlist.append(o);
}

void Options::setHelpText(const QString &s) { helptext = s; }

QString Options::getHelpText() { return helptext; }

QString Options::getProgramName() { return progname; }

QStringList Options::getFileList() { return filelist; }

bool Options::isOn(const QString &s)
{
    for (int i = 0; i < optlist.size(); ++i)
        if (optlist[i].getName() == s && optlist[i].isActive())
            return true;
    return false;
}

QString Options::getArg(const QString &s)
{
    for (int i = 0; i < optlist.size(); ++i)
        if (optlist[i].getName() == s)
            return optlist[i].getArg();
    return QString();
}
