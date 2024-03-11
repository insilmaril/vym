#include "confluence-user.h"

void ConfluenceUser::copy(const ConfluenceUser &other) {
    title = other.title;
    url = other.url;
    userName = other.userName;
    displayName = other.displayName;
    userKey = other.userKey;
};

void ConfluenceUser::setTitle(const QString &s) {title = s;}
void ConfluenceUser::setUrl(const QString &s) {url = s;}
void ConfluenceUser::setUserName(const QString &s) {userName = s;}
void ConfluenceUser::setDisplayName(const QString &s) {displayName = s;}
void ConfluenceUser::setUserKey(const QString &s) {userKey = s;}

QString ConfluenceUser::getTitle() {return title;}
QString ConfluenceUser::getUrl() {return url;}
QString ConfluenceUser::getUserName() {return userName;}
QString ConfluenceUser::getDisplayName() {return displayName;}
QString ConfluenceUser::getUserKey() {return userKey;}

