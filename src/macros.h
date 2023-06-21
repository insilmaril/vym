#ifndef MACROS_H
#define MACROS_H

#include <QString>

class Macros {
  public:
    QString getPath();
    bool setPath(const QString&);
    bool pathExists(const QString&);
    QString get();
    QString help();

  private:
    QString macrosPath;
};
#endif
