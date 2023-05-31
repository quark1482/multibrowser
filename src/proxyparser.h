#ifndef PROXYPARSER_H
#define PROXYPARSER_H

#include <QtNetwork>

class ProxyParser {
public:
    static QString       getTextFromProxy(QNetworkProxy);
    static QNetworkProxy getProxyFromText(QString);
};

#endif // PROXYPARSER_H
