#ifndef MULTIBROWSER_H
#define MULTIBROWSER_H

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>
#include <QMainWindow>
#include <QApplication>
#include "proxyparser.h"

using LinkRecord=struct {
    QUrl    urlLink;
    bool    bBusy;
    uint    uiIndex,
            uiHits,
            uiErrors;
    QString sLastError;
};

using LinkList=QVector<LinkRecord>;

using ProxyRecord=struct {
    QNetworkProxy npxProxy;
    bool          bBusy;
    uint          uiIndex,
                  uiHits,
                  uiErrors;
};

using ProxyList=QVector<ProxyRecord>;

class MultiBrowser:public QMainWindow {
    Q_OBJECT
public:
    MultiBrowser(QWidget * =nullptr);
    ~MultiBrowser();
private:
    void      browse();
    LinkList  getLinksFromText(QString);
    ProxyList getProxiesFromText(QString);
    QString   getTextFileContents(QString);
    QString   getTextFromLinks(LinkList);
    QString   getTextFromProxies(ProxyList);
private slots:
    void loadLinksClicked(bool);
    void loadProxiesClicked(bool);
    void runClicked(bool);
    void workerFinished();
    void workerStarted();
    void workerStatusChanged(QString);
private:
    bool           bRunning;
    uint           uiTotalWorkers;
    LinkList       llCurrentLinks;
    ProxyList      plCurrentProxies;
    // UI widgets go here:
    QWidget        wgtMain;
        QVBoxLayout    vblMain;
            QTabWidget     tbwMain;
                QWidget        wgtSettings;
                    QVBoxLayout    vblSettings;
                        QVBoxLayout    vblLinks;
                            QHBoxLayout    hblLinks;
                                QLabel         lblLinks;
                                QPushButton    btnLoadLinks;
                            QPlainTextEdit txtLinks;
                        QVBoxLayout    vblProxies;
                            QHBoxLayout    hblProxies;
                                QLabel         lblProxies;
                                QPushButton    btnLoadProxies;
                            QPlainTextEdit txtProxies;
                        QHBoxLayout    hblOptions;
                            QHBoxLayout    hblOptionThreads;
                                QLabel         lblThreads;
                                QSpinBox       spbThreads;
                            QHBoxLayout    hblOptionCooldown;
                                QLabel         lblCooldown;
                                QSpinBox       spbCooldown;
                            QHBoxLayout    hblOptionUse;
                                QRadioButton   optUseBrowser;
                                QRadioButton   optUseHTTP;
                QWidget        wgtProgress;
                    QVBoxLayout    vblProgress;
                        QVBoxLayout    vblLinkStats;
                            QLabel         lblLinkStats;
                            QTableWidget   twgLinkStats;
                        QVBoxLayout    vblProxyStats;
                            QLabel         lblProxyStats;
                            QTableWidget   twgProxyStats;
            QHBoxLayout    hblRun;
                QPushButton    btnRun;
    QStatusBar     stbMain;
};

class BrowserWorker:public QThread {
    Q_OBJECT
public:
    using RunMode=enum {
        RM_WEB_ENGINE,
        RM_NETWORK
    };
    BrowserWorker(QObject * =nullptr,LinkRecord * =nullptr,ProxyRecord * =nullptr);
    LinkRecord  *getLinkRecord();
    ProxyRecord *getProxyRecord();
    void        run() override;
    void        setCooldown(uint);
    void        setMode(RunMode);
signals:
    void statusChanged(QString);
private:
    uint        uiCooldown;
    QString     sError;
    LinkRecord  *lrLink;
    ProxyRecord *prProxy;
    RunMode     rmMode;
    void runWithWebEngine();
    void runWithNetwork();
    void showCurrentIP(QString);
};

#endif // MULTIBROWSER_H
