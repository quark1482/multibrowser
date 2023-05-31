#include "proxyparser.h"

QString ProxyParser::getTextFromProxy(QNetworkProxy npxProxy) {
    QString sResult=QString(),
            sScheme=QString();
    if(QNetworkProxy::ProxyType::HttpProxy==npxProxy.type())
        sScheme=QStringLiteral("http");
    else if(QNetworkProxy::ProxyType::Socks5Proxy==npxProxy.type())
        sScheme=QStringLiteral("socks");
    if(!sScheme.isEmpty()) {
        QString sAuth=QString();
        sResult.append(QStringLiteral("%1://").arg(sScheme));
        if(!npxProxy.user().isEmpty())
            sAuth.append(npxProxy.user());
        if(!npxProxy.password().isEmpty())
            sAuth.append(QStringLiteral(":%1").arg(npxProxy.password()));
        if(!sAuth.isEmpty())
            sAuth.append(QStringLiteral("@"));
        sResult.append(sAuth);
        sResult.append(npxProxy.hostName());
        sResult.append(
            QStringLiteral(":%1").arg(npxProxy.port())
        );
    }
    return sResult;
}

QNetworkProxy ProxyParser::getProxyFromText(QString sProxy) {
    QNetworkProxy npxResult(QNetworkProxy::ProxyType::NoProxy);
    QUrl          urlTestProxy;
    urlTestProxy.setUrl(sProxy.trimmed());
    if(urlTestProxy.isValid()) {
        int     iType=QNetworkProxy::ProxyType::NoProxy;
        QString sScheme=urlTestProxy.scheme().toLower(),
                sHost=urlTestProxy.host(),
                sUser=urlTestProxy.userName(),
                sPass=urlTestProxy.password();
        if(sScheme==QStringLiteral("http")||sScheme==QStringLiteral("https"))
            iType=QNetworkProxy::ProxyType::HttpProxy;
        else if(sScheme==QStringLiteral("socks"))
            iType=QNetworkProxy::ProxyType::Socks5Proxy;
        if(QNetworkProxy::ProxyType::NoProxy!=iType) {
            int iPort=urlTestProxy.port();
            if(iPort>0) {
                npxResult.setType(
                    static_cast<QNetworkProxy::ProxyType>(iType)
                );
                npxResult.setHostName(sHost);
                npxResult.setPort(iPort);
                npxResult.setUser(sUser);
                npxResult.setPassword(sPass);
            }
        }
    }
    return npxResult;
}
