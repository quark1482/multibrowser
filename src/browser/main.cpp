#include <QtCore>
#include <QtWebEngineCore>
#include <QApplication>
#include <QCommandLineParser>
#include "../proxyparser.h"

class UrlResponseInterceptor:public QWebEngineUrlResponseInterceptor {
public:
    QJsonObject getHeaders() {
        return jsnObj;
    }
    void interceptResponseHeaders(QWebEngineUrlResponseInfo &webUrlRIInfo) override {
        // Processes the response only if it's the top-level request.
        if(jsnObj.isEmpty())
            // Creates a JSON object including all headers as name:value pairs.
            for(const auto &k:webUrlRIInfo.responseHeaders().keys())
                jsnObj[k]=QString(webUrlRIInfo.responseHeaders().value(k));
    }
private:
    QJsonObject jsnObj;
};

bool browse(QUrl          urlURL,
            QNetworkProxy npxProxy,
            QString       sAgent,
            QString       &sJSON) {
    bool                   bResult;
    QString                sContents;
    QWebEnginePage         webPage;
    QEventLoop             evlBrowse;
    QJsonObject            jsnParams,
                           jsnResponse;
    UrlResponseInterceptor uriInterceptor;
    sJSON.clear();
    // Includes the passed parameters in the response as well.
    jsnParams[QStringLiteral("url")]=urlURL.toString();
    jsnParams[QStringLiteral("proxy")]=ProxyParser::getTextFromProxy(npxProxy);
    jsnParams[QStringLiteral("agent")]=sAgent;
    jsnResponse[QStringLiteral("params")]=jsnParams;
    // Adds a fake password when the proxy only requires the user name.
    // QtNetwork seems to misbehave in this particularly specific case.
    if(!npxProxy.user().isEmpty()&&npxProxy.password().isEmpty())
        npxProxy.setPassword(npxProxy.user());
    QNetworkProxy::setApplicationProxy(npxProxy);
    if(!sAgent.isEmpty())
        webPage.profile()->setHttpUserAgent(sAgent);
    QObject::connect(
        &webPage,
        &QWebEnginePage::certificateError,
        [](const QWebEngineCertificateError &webErr) {
            // Accepts every certificate, ignoring the possible errors, ...
            // ... and makes the program able to use non-HTTPS proxies.
            const_cast<QWebEngineCertificateError &>(webErr).acceptCertificate();
        }
    );
    QObject::connect(
        &webPage,
        &QWebEnginePage::loadFinished,
        [&](bool bOK) {
            if(bOK) {
                QEventLoop evlContent;
                webPage.toHtml(
                    [&](const QString &sHTML) {
                        sContents=sHTML;
                        evlContent.exit();
                    }
                );
                // Waits until the page HTML contents are fully available.
                evlContent.exec();
            }
            evlBrowse.exit(bOK);
        }
    );
    webPage.setUrlResponseInterceptor(&uriInterceptor);
    webPage.load(urlURL);
    // Waits until the web page is fully loaded (or some error occurs).
    if(bResult=evlBrowse.exec()) {
        jsnResponse[QStringLiteral("headers")]=uriInterceptor.getHeaders();
        jsnResponse[QStringLiteral("content")]=sContents;
    }
    else
        jsnResponse[QStringLiteral("error")]=QStringLiteral("Unable to load the URL");
    sJSON=QJsonDocument(jsnResponse).toJson();
    return bResult;
}

bool parseParams(QUrl          &urlURL,
                 QNetworkProxy &npxProxy,
                 QString       &sAgent,
                 QString       &sError) {
    QString            sURL,
                       sProxy;
    QCommandLineParser clpParser;
    urlURL.clear();
    npxProxy=QNetworkProxy();
    sAgent.clear();
    sError.clear();
    clpParser.setApplicationDescription(
        QStringLiteral("Browses to the given URL and returns a JSON-encoded response")
    );
    clpParser.addHelpOption();
    clpParser.addPositionalArgument(
        QStringLiteral("url"),
        QStringLiteral("URL resource to browse")
    );
    clpParser.addOption(
        {
            {
                QStringLiteral("p"),
                QStringLiteral("proxy")
            },
            QStringLiteral("Connect through Proxy"),
            QStringLiteral("proxy")
        }
    );
    clpParser.addOption(
        {
            {
                QStringLiteral("a"),
                QStringLiteral("agent")
            },
            QStringLiteral("Spoof with User-Agent"),
            QStringLiteral("agent")
        }
    );
    // Encapsulates the making of an error message, immediately followed ...
    // ... by the application's description and usage, handly when a lot ...
    // ... of parameter validations are necessary (when it's called with ...
    // ... no arguments, the QCommandLineParser::parse() error is shown).
    auto fnMakeErrMsg=[&clpParser](std::optional<QString> oErr=std::nullopt) {
        return QStringLiteral("Error: %1\n\n%2").arg(
            std::nullopt!=oErr?oErr.value():clpParser.errorText(),
            clpParser.helpText()
        );
    };
    if(!clpParser.parse(QApplication::arguments())) {
        sError=fnMakeErrMsg();
        return false;
    }
    if(!clpParser.positionalArguments().count()) {
        sError=fnMakeErrMsg(QStringLiteral("Missing URL"));
        return false;
    }
    sURL=clpParser.positionalArguments().first();
    urlURL=QUrl(sURL);
    if(!urlURL.isValid()) {
        sError=fnMakeErrMsg(QStringLiteral("Invalid URL"));
        return false;
    }
    sProxy=clpParser.value(QStringLiteral("proxy"));
    npxProxy=ProxyParser::getProxyFromText(sProxy);
    if(!sProxy.isEmpty()&&QNetworkProxy::ProxyType::NoProxy==npxProxy.type()) {
        sError=fnMakeErrMsg(QStringLiteral("Invalid proxy"));
        return false;
    }
    sAgent=clpParser.value(QStringLiteral("agent"));
    return true;
}

int main(int argc,char *argv[]) {
    // Creates a widgets app, since QtWebEngine does not work in console.
    QApplication appMain(argc,argv);
    // Encapsulates the 'console' code inside a lambda, so it's called ...
    // ... immediately after the application enters the main event loop.
    // This is not required at all (a simple use of 'return EXIT_' can ...
    // ... achieve the same functionality). It's just showing a way of ...
    // using QApplication::exec() without creating any widget, because ...
    // ... otherwise, the application would never reach an exit point.
    QTimer::singleShot(
        0,
        [&appMain]() {
            QString           sAgent,
                              sError,
                              sJSON;
            QUrl              urlURL;
            QNetworkProxy     npxProxy;
            QTextStream       tstOutput(stdout);
            // Shows the results (either an error or a JSON response, and exits.
            // The use of QTextStream is an alternative to 'std::cout', with ...
            // ... the plus of not having to do 'toStdString()' conversionss.
            if(!parseParams(urlURL,npxProxy,sAgent,sError)) {
                tstOutput << sError;
                appMain.exit(EXIT_FAILURE);
            }
            else if(!browse(urlURL,npxProxy,sAgent,sJSON)) {
                tstOutput << sJSON;
                appMain.exit(EXIT_FAILURE);
            }
            else {
                tstOutput << sJSON;
                appMain.exit(EXIT_SUCCESS);
            }
        }
    );
    return appMain.exec();
}
