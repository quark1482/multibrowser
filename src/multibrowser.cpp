#include "multibrowser.h"

#define APP_BROWSER_EXE  "Browser.exe"

#define FILTER_TXT_FILES "Text files (*.txt)"

#define MAX_THREADS  16
#define MAX_COOLDOWN 60

#define LABELS_LINK_STATS { \
    QStringLiteral("Link"), \
    QStringLiteral("Status"), \
    QStringLiteral("Hits"), \
    QStringLiteral("Errors"), \
    QStringLiteral("Last error") \
}

#define LABELS_PROXY_STATS { \
    QStringLiteral("Proxy"), \
    QStringLiteral("Hits"), \
    QStringLiteral("Errors") \
}

#define COLOR_ACTIVE_LINK  0x99FFFF
#define COLOR_ACTIVE_PROXY 0xFF99FF

enum LinkStatsTableColumns {
    LSTC_LINK,
    LSTC_STATUS,
    LSTC_HITS,
    LSTC_ERRORS,
    LSTC_LAST_ERROR,
    LSTC_TOTAL
};

enum ProxyStatsTableColumns {
    PSTC_PROXY,
    PSTC_HITS,
    PSTC_ERRORS,
    PSTC_TOTAL
};

MultiBrowser::MultiBrowser(QWidget *wgtParent):
QMainWindow(wgtParent) {
    bRunning=false;
    uiTotalWorkers=0;
    llCurrentLinks.clear();
    plCurrentProxies.clear();
    // UI setup goes here:
    [=]() {
        std::function<void(QTableWidget *,uint,QStringList)> fnConfigTable=[](
            QTableWidget *twgTable,
            uint         uiTotalColumns,
            QStringList  slColumnLabels) {
                twgTable->setColumnCount(uiTotalColumns);
                twgTable->setHorizontalHeaderLabels(slColumnLabels);
                twgTable->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::ResizeMode::ResizeToContents
                );
                twgTable->horizontalHeader()->setSectionResizeMode(
                    0,
                    QHeaderView::ResizeMode::Stretch
                );
                twgTable->verticalHeader()->setSectionResizeMode(
                    QHeaderView::ResizeMode::Fixed
                );
                twgTable->setEditTriggers(
                    QAbstractItemView::EditTrigger::NoEditTriggers
                );
                twgTable->setSelectionBehavior(
                    QAbstractItemView::SelectionBehavior::SelectRows
                );
                twgTable->setSelectionMode(
                    QAbstractItemView::SelectionMode::SingleSelection
                );
            };

        wgtMain.setLayout(&vblMain);
        vblMain.addWidget(&tbwMain);

        tbwMain.addTab(&wgtSettings,QStringLiteral("Settings"));
        wgtSettings.setLayout(&vblSettings);

        vblSettings.addLayout(&vblLinks);
        vblLinks.addLayout(&hblLinks);
        lblLinks.setSizePolicy(
            QSizePolicy::Policy::Expanding,
            QSizePolicy::Policy::Preferred
        );
        lblLinks.setText(QStringLiteral("List of links:"));
        hblLinks.addWidget(&lblLinks);
        btnLoadLinks.setText(QStringLiteral("Load"));
        hblLinks.addWidget(&btnLoadLinks);
        vblLinks.addWidget(&txtLinks);

        vblSettings.addLayout(&vblProxies);
        vblProxies.addLayout(&hblProxies);
        lblProxies.setSizePolicy(
            QSizePolicy::Policy::Expanding,
            QSizePolicy::Policy::Preferred
        );
        lblProxies.setText(QStringLiteral("List of proxies:"));
        hblProxies.addWidget(&lblProxies);
        btnLoadProxies.setText(QStringLiteral("Load"));
        hblProxies.addWidget(&btnLoadProxies);
        vblProxies.addWidget(&txtProxies);

        vblSettings.addLayout(&hblOptions);
        hblOptions.addStretch();
        hblOptions.addLayout(&hblOptionThreads);
        lblThreads.setText(QStringLiteral("Max. threads:"));
        hblOptionThreads.addWidget(&lblThreads);
        spbThreads.setMinimum(1);
        spbThreads.setMaximum(MAX_THREADS);
        hblOptionThreads.addWidget(&spbThreads);
        hblOptions.addStretch();
        hblOptions.addLayout(&hblOptionCooldown);
        lblCooldown.setText(QStringLiteral("Max. cooldown:"));
        hblOptionCooldown.addWidget(&lblCooldown);
        spbCooldown.setMinimum(0);
        spbCooldown.setMaximum(MAX_COOLDOWN);
        spbCooldown.setSuffix(QStringLiteral(" s"));
        hblOptionCooldown.addWidget(&spbCooldown);
        hblOptions.addStretch();
        hblOptions.addLayout(&hblOptionUse);
        optUseBrowser.setText(QStringLiteral("Use browser"));
        hblOptionUse.addWidget(&optUseBrowser);
        optUseHTTP.setText(QStringLiteral("Use HTTP"));
        optUseHTTP.setChecked(true);
        hblOptionUse.addWidget(&optUseHTTP);

        tbwMain.addTab(&wgtProgress,QStringLiteral("Progress"));
        wgtProgress.setLayout(&vblProgress);
        vblProgress.addLayout(&vblLinkStats);
        lblLinkStats.setText(QStringLiteral("Link stats:"));
        lblLinkStats.setAlignment(Qt::AlignmentFlag::AlignCenter);
        vblLinkStats.addWidget(&lblLinkStats);
        fnConfigTable(&twgLinkStats,LSTC_TOTAL,LABELS_LINK_STATS);
        vblLinkStats.addWidget(&twgLinkStats);
        vblProgress.addLayout(&vblProxyStats);
        lblProxyStats.setText(QStringLiteral("Proxy stats:"));
        lblProxyStats.setAlignment(Qt::AlignmentFlag::AlignCenter);
        vblProxyStats.addWidget(&lblProxyStats);
        fnConfigTable(&twgProxyStats,PSTC_TOTAL,LABELS_PROXY_STATS);
        vblProxyStats.addWidget(&twgProxyStats);

        vblMain.addLayout(&hblRun);
        hblRun.addStretch();
        btnRun.setText(QStringLiteral("Run"));
        hblRun.addWidget(&btnRun);

        this->setCentralWidget(&wgtMain);
        this->setMinimumSize(800,600);
        this->setStatusBar(&stbMain);

        connect(
            &btnLoadLinks,
            &QPushButton::clicked,
            this,
            &MultiBrowser::loadLinksClicked
        );
        connect(
            &btnLoadProxies,
            &QPushButton::clicked,
            this,
            &MultiBrowser::loadProxiesClicked
        );
        connect(
            &btnRun,
            &QPushButton::clicked,
            this,
            &MultiBrowser::runClicked
        );
    }();
}

MultiBrowser::~MultiBrowser() {
    // No cleanup required ATM.
}

void MultiBrowser::browse() {
    while(uiTotalWorkers<(uint)spbThreads.value()) {
        bool bFreeLinks=false,
             bFreeProxies=false;
        // Verifies that thee are non-busy links.
        for(const auto &l:llCurrentLinks)
            if(!l.bBusy) {
                bFreeLinks=true;
                break;
            }
        if(bFreeLinks) {
            int           iRandomLink,
                          iRandomProxy;
            LinkRecord    *lrSelectedLink=nullptr;
            ProxyRecord   *prSelectedProxy=nullptr;
            BrowserWorker *bwWorker=nullptr;
            uiTotalWorkers++;
            // Picks one non-busy link at random.
            while(true) {
                iRandomLink=QRandomGenerator::global()->bounded(
                    llCurrentLinks.count()
                );
                if(!llCurrentLinks.at(iRandomLink).bBusy)
                    break;
            }
            lrSelectedLink=&llCurrentLinks[iRandomLink];
            if(!plCurrentProxies.isEmpty()) {
                // Verifies that there are non-busy proxies.
                for(const auto &p:plCurrentProxies)
                    if(!p.bBusy) {
                        bFreeProxies=true;
                        break;
                    }
                if(bFreeProxies)
                    // Picks one non-busy proxy at random.
                    while(true) {
                        iRandomProxy=QRandomGenerator::global()->bounded(
                            plCurrentProxies.count()
                        );
                        if(!plCurrentProxies.at(iRandomProxy).bBusy)
                            break;
                    }
                else
                    // Picks any if all proxies are busy.
                    iRandomProxy=QRandomGenerator::global()->bounded(
                        plCurrentProxies.count()
                    );
                prSelectedProxy=&plCurrentProxies[iRandomProxy];
            }
            bwWorker=new BrowserWorker(this,lrSelectedLink,prSelectedProxy);
            bwWorker->setCooldown(
                QRandomGenerator::global()->bounded(spbCooldown.value()+1)
            );
            if(optUseHTTP.isChecked())
                bwWorker->setMode(BrowserWorker::RunMode::RM_NETWORK);
            else if(optUseBrowser.isChecked())
                bwWorker->setMode(BrowserWorker::RunMode::RM_WEB_ENGINE);
            connect(
                bwWorker,
                &BrowserWorker::started,
                this,
                &MultiBrowser::workerStarted
            );
            connect(
                bwWorker,
                &BrowserWorker::finished,
                this,
                &MultiBrowser::workerFinished
            );
            connect(
                bwWorker,
                &BrowserWorker::statusChanged,
                this,
                &MultiBrowser::workerStatusChanged
            );
            bwWorker->start();
        }
        else
            break; // Nothing to do if all links are busy.
    }
}

LinkList MultiBrowser::getLinksFromText(QString sText) {
    uint        uiIndex=0;
    LinkList    llResult={};
    QStringList slLinks=sText.split(
        QStringLiteral("\n"),
        Qt::SplitBehaviorFlags::SkipEmptyParts
    );
    for(const auto &s:slLinks) {
        QUrl urlTestLink;
        urlTestLink.setUrl(s.trimmed());
        if(urlTestLink.isValid()) {
            QString sScheme=urlTestLink.scheme().toLower();
            if(sScheme==QStringLiteral("http")||sScheme==QStringLiteral("https")) {
                LinkRecord lrLink;
                lrLink.urlLink=urlTestLink;
                lrLink.bBusy=false;
                lrLink.uiIndex=uiIndex++;
                lrLink.uiHits=0;
                lrLink.uiErrors=0;
                lrLink.sLastError=QString();
                llResult.append(lrLink);
            }
        }
    }
    return llResult;
}

ProxyList MultiBrowser::getProxiesFromText(QString sText) {
    uint        uiIndex=0;
    ProxyList   plResult={};
    QStringList slProxies=sText.split(
        QStringLiteral("\n"),
        Qt::SplitBehaviorFlags::SkipEmptyParts
    );
    for(const auto &s:slProxies) {
        QNetworkProxy npxProxy=ProxyParser::getProxyFromText(s);
        if(QNetworkProxy::ProxyType::NoProxy!=npxProxy.type()) {
            ProxyRecord prProxy;
            prProxy.npxProxy=npxProxy;
            prProxy.bBusy=false;
            prProxy.uiIndex=uiIndex++;
            prProxy.uiHits=0;
            prProxy.uiErrors=0;
            plResult.append(prProxy);
        }
    }
    return plResult;
}

QString MultiBrowser::getTextFileContents(QString sPrompt) {
    QString sResult=QString(),
            sDefaultFolder,
            sTempPath;
    sDefaultFolder=QStandardPaths::standardLocations(
        QStandardPaths::StandardLocation::DocumentsLocation
    ).at(0);
    sTempPath=QFileDialog::getOpenFileName(
        this,
        sPrompt,
        sDefaultFolder,
        QStringLiteral(FILTER_TXT_FILES)
    );
    if(!sTempPath.isEmpty()) {
        QFile fFile;
        fFile.setFileName(sTempPath);
        if(fFile.open(QFile::OpenModeFlag::ReadOnly)) {
            sResult=fFile.readAll();
            fFile.close();
        }
        else
            QMessageBox::critical(
                this,
                QStringLiteral("Error"),
                fFile.errorString()
            );
    }
    return sResult;
}

QString MultiBrowser::getTextFromLinks(LinkList llLinks) {
    QString sResult=QString();
    for(const auto &l:llLinks) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral("\n"));
        sResult.append(l.urlLink.toString());
    }
    return sResult;
}

QString MultiBrowser::getTextFromProxies(ProxyList plProxies) {
    QString sResult=QString();
    for(const auto &p:plProxies) {
        QString sProxy=ProxyParser::getTextFromProxy(p.npxProxy);
        if(!sProxy.isEmpty()) {
            if(!sResult.isEmpty())
                sResult.append(QStringLiteral("\n"));
            sResult.append(sProxy);
        }
    }
    return sResult;
}

void MultiBrowser::loadLinksClicked(bool) {
    txtLinks.setPlainText(this->getTextFileContents(lblLinks.text()));
}

void MultiBrowser::loadProxiesClicked(bool) {
    txtProxies.setPlainText(this->getTextFileContents(lblProxies.text()));
}

void MultiBrowser::runClicked(bool) {
    if(bRunning) {
        bRunning=false;
        btnRun.setEnabled(false);
        stbMain.showMessage(QStringLiteral("Stopping (wait)..."));
        while(uiTotalWorkers)
            QApplication::processEvents(
                QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents
            );
        stbMain.clearMessage();
        btnRun.setEnabled(true);
        btnRun.setText(QStringLiteral("Run"));
    }
    else {
        llCurrentLinks=this->getLinksFromText(txtLinks.toPlainText());
        plCurrentProxies=this->getProxiesFromText(txtProxies.toPlainText());
        txtLinks.setPlainText(this->getTextFromLinks(llCurrentLinks));
        txtProxies.setPlainText(this->getTextFromProxies(plCurrentProxies));
        if(llCurrentLinks.isEmpty())
            QMessageBox::critical(
                this,
                QStringLiteral("Error"),
                QStringLiteral("At least one link is required")
            );
        else {
            bRunning=true;
            twgLinkStats.clearContents();
            twgLinkStats.setRowCount(llCurrentLinks.count());
            for(const auto &l:llCurrentLinks) {
                QTableWidgetItem *twiItem;
                twiItem=new QTableWidgetItem(l.urlLink.url());
                twgLinkStats.setItem(l.uiIndex,LSTC_LINK,twiItem);
                twiItem=new QTableWidgetItem(QString());
                twgLinkStats.setItem(l.uiIndex,LSTC_STATUS,twiItem);
                twiItem=new QTableWidgetItem(QString::number(l.uiHits));
                twiItem->setTextAlignment(
                    Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignVCenter
                );
                twgLinkStats.setItem(l.uiIndex,LSTC_HITS,twiItem);
                twiItem=new QTableWidgetItem(QString::number(l.uiErrors));
                twiItem->setTextAlignment(
                    Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignVCenter
                );
                twgLinkStats.setItem(l.uiIndex,LSTC_ERRORS,twiItem);
                twiItem=new QTableWidgetItem(QString());
                twgLinkStats.setItem(l.uiIndex,LSTC_LAST_ERROR,twiItem);
            }
            twgProxyStats.clearContents();
            twgProxyStats.setRowCount(plCurrentProxies.count());
            for(const auto &p:plCurrentProxies) {
                QTableWidgetItem *twiItem;
                twiItem=new QTableWidgetItem(ProxyParser::getTextFromProxy(p.npxProxy));
                twgProxyStats.setItem(p.uiIndex,PSTC_PROXY,twiItem);
                twiItem=new QTableWidgetItem(QString::number(p.uiHits));
                twiItem->setTextAlignment(
                    Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignVCenter
                );
                twgProxyStats.setItem(p.uiIndex,PSTC_HITS,twiItem);
                twiItem=new QTableWidgetItem(QString::number(p.uiErrors));
                twiItem->setTextAlignment(
                    Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignVCenter
                );
                twgProxyStats.setItem(p.uiIndex,PSTC_ERRORS,twiItem);
            }
            btnRun.setText(QStringLiteral("Stop"));
            stbMain.showMessage(QStringLiteral("Running..."));
            tbwMain.setCurrentWidget(&wgtProgress);
            this->browse();
        }
    }
}

void MultiBrowser::workerFinished() {
    BrowserWorker *bwWorker=qobject_cast<BrowserWorker *>(QObject::sender());
    LinkRecord    *lrCurrentLink=bwWorker->getLinkRecord();
    ProxyRecord   *prCurrentProxy=bwWorker->getProxyRecord();
    lrCurrentLink->bBusy=false;
    for(int iK=0;iK<twgLinkStats.columnCount();iK++)
        twgLinkStats.item(
            lrCurrentLink->uiIndex,
            iK
        )->setBackground(QBrush(Qt::GlobalColor::white));
    twgLinkStats.scrollToItem(twgLinkStats.item(lrCurrentLink->uiIndex,0));
    if(nullptr!=prCurrentProxy) {
        prCurrentProxy->bBusy=false;
        for(int iK=0;iK<twgProxyStats.columnCount();iK++)
            twgProxyStats.item(
                prCurrentProxy->uiIndex,
                iK
            )->setBackground(QBrush(Qt::GlobalColor::white));
        twgProxyStats.scrollToItem(twgProxyStats.item(prCurrentProxy->uiIndex,0));
    }
    bwWorker->disconnect();
    bwWorker->deleteLater();
    uiTotalWorkers--;
    if(bRunning)
        this->browse();
}

void MultiBrowser::workerStarted() {
    BrowserWorker *bwWorker=qobject_cast<BrowserWorker *>(QObject::sender());
    LinkRecord    *lrCurrentLink=bwWorker->getLinkRecord();
    ProxyRecord   *prCurrentProxy=bwWorker->getProxyRecord();
    lrCurrentLink->bBusy=true;
    for(int iK=0;iK<twgLinkStats.columnCount();iK++)
        twgLinkStats.item(
            lrCurrentLink->uiIndex,
            iK
        )->setBackground(QBrush(QColor(COLOR_ACTIVE_LINK)));
    twgLinkStats.scrollToItem(twgLinkStats.item(lrCurrentLink->uiIndex,0));
    if(nullptr!=prCurrentProxy) {
        prCurrentProxy->bBusy=true;
        for(int iK=0;iK<twgProxyStats.columnCount();iK++)
            twgProxyStats.item(
                prCurrentProxy->uiIndex,
                iK
            )->setBackground(QBrush(QColor(COLOR_ACTIVE_PROXY)));
        twgProxyStats.scrollToItem(twgProxyStats.item(prCurrentProxy->uiIndex,0));
    }
}

void MultiBrowser::workerStatusChanged(QString sStatus) {
    BrowserWorker *bwWorker=qobject_cast<BrowserWorker *>(QObject::sender());
    LinkRecord    *lrCurrentLink=bwWorker->getLinkRecord();
    ProxyRecord   *prCurrentProxy=bwWorker->getProxyRecord();
    twgLinkStats.item(lrCurrentLink->uiIndex,LSTC_STATUS)->setText(
        sStatus
    );
    twgLinkStats.item(lrCurrentLink->uiIndex,LSTC_HITS)->setText(
        QString::number(lrCurrentLink->uiHits)
    );
    twgLinkStats.item(lrCurrentLink->uiIndex,LSTC_ERRORS)->setText(
        QString::number(lrCurrentLink->uiErrors)
    );
    twgLinkStats.item(lrCurrentLink->uiIndex,LSTC_LAST_ERROR)->setText(
        lrCurrentLink->sLastError
    );
    if(nullptr!=prCurrentProxy) {
        twgProxyStats.item(prCurrentProxy->uiIndex,PSTC_HITS)->setText(
            QString::number(prCurrentProxy->uiHits)
        );
        twgProxyStats.item(prCurrentProxy->uiIndex,PSTC_ERRORS)->setText(
            QString::number(prCurrentProxy->uiErrors)
        );
    }
}

BrowserWorker::BrowserWorker(QObject     *objParent,
                             LinkRecord  *lrNewLink,
                             ProxyRecord *prNewProxy):
QThread(objParent) {
    uiCooldown=0;
    sError.clear();
    lrLink=lrNewLink;
    prProxy=prNewProxy;
    rmMode=BrowserWorker::RunMode::RM_NETWORK;
}

LinkRecord *BrowserWorker::getLinkRecord() {
    return lrLink;
}

ProxyRecord *BrowserWorker::getProxyRecord() {
    return prProxy;
}

void BrowserWorker::run() {
    emit statusChanged(QString());
    if(nullptr!=lrLink) {
        for(int iK=uiCooldown;iK;iK--) {
            emit statusChanged(QStringLiteral("Starting in %1s").arg(iK));
            QThread::sleep(1);
        }
        emit statusChanged(QStringLiteral("Browsing..."));
        if(BrowserWorker::RunMode::RM_NETWORK==rmMode)
            this->runWithNetwork();
        else if(BrowserWorker::RunMode::RM_WEB_ENGINE==rmMode)
            this->runWithWebEngine();
    }
    if(nullptr!=lrLink) {
        if(sError.isEmpty())
            lrLink->uiHits++;
        else {
            lrLink->uiErrors++;
            lrLink->sLastError=sError;
        }
        lrLink->bBusy=false;
    }
    if(nullptr!=prProxy) {
        if(sError.isEmpty())
            prProxy->uiHits++;
        else
            prProxy->uiErrors++;
        prProxy->bBusy=false;
    }
    emit statusChanged(QStringLiteral("Idle"));
}

void BrowserWorker::runWithWebEngine() {
    sError.clear();
    if(nullptr!=lrLink) {
        QString     sBrowserPath;
        QStringList slBrowserParams;
        QProcess    proBrowserApp;
        sBrowserPath=QStringLiteral("%1/%2").arg(
            QApplication::applicationDirPath(),
            QStringLiteral(APP_BROWSER_EXE)
        );
        slBrowserParams={lrLink->urlLink.toString()};
        if(nullptr!=prProxy)
            slBrowserParams.append({
                QStringLiteral("-p"),
                ProxyParser::getTextFromProxy(prProxy->npxProxy)
            });
        proBrowserApp.start(sBrowserPath,slBrowserParams);
        proBrowserApp.waitForFinished(-1);
        if(QProcess::ExitStatus::NormalExit==proBrowserApp.exitStatus()) {
            QString       sJSON=proBrowserApp.readAll();
            QJsonDocument jsnDoc=QJsonDocument::fromJson(sJSON.toUtf8());
            QJsonObject   jsnObj=jsnDoc.isObject()?jsnDoc.object():QJsonObject();
            if(!proBrowserApp.exitCode())
                if(jsnObj.contains(QStringLiteral("content")))
                    this->showCurrentIP(
                        jsnObj.value(QStringLiteral("content")).toString()
                    );
                else
                    sError=QStringLiteral("Wrong browser response"); // Impossible.
            else
                if(jsnObj.contains(QStringLiteral("error")))
                    sError=jsnObj.value(QStringLiteral("error")).toString();
                else
                    sError=QStringLiteral("Wrong browser call"); // Impossible.
        }
        else
            sError=QStringLiteral("Browser crashed"); // Improbable.
    }
}

void BrowserWorker::runWithNetwork() {
    sError.clear();
    if(nullptr!=lrLink) {
        uint                  uiStatus;
        QNetworkAccessManager namManager;
        QNetworkRequest       nrqRequest;
        QNetworkReply         *nrpReply;
        connect(
            &namManager,
            &QNetworkAccessManager::sslErrors,
            [](QNetworkReply *nrpError,const QList<QSslError> &) {
                nrpError->ignoreSslErrors();
            }
        );
        if(nullptr!=prProxy)
            namManager.setProxy(prProxy->npxProxy);
        namManager.setTransferTimeout();
        nrqRequest.setUrl(lrLink->urlLink);
        nrpReply=namManager.get(nrqRequest);
        while(!nrpReply->isFinished())
            QApplication::processEvents();
        uiStatus=nrpReply->attribute(
            QNetworkRequest::Attribute::HttpStatusCodeAttribute
        ).toUInt();
        if(QNetworkReply::NetworkError::NoError!=nrpReply->error())
            if(uiStatus)
                sError=QStringLiteral("Unexpected response code: %1").arg(uiStatus);
            else
                sError=nrpReply->errorString();
        else
            if(uiStatus)
                this->showCurrentIP(nrpReply->readAll());
            else
                sError=QStringLiteral("Response timeout expired");
        nrpReply->~QNetworkReply();
    }
}

void BrowserWorker::showCurrentIP(QString sHTML) {
    if(nullptr!=lrLink&&nullptr!=prProxy) {
        if(!lrLink->urlLink.host().compare(
            QStringLiteral("www.ipchicken.com"),
            Qt::CaseSensitivity::CaseInsensitive
        )) {
            QRegularExpression      rxIP;
            QRegularExpressionMatch rxmIP;
            rxIP.setPattern(QStringLiteral("Name\\nAddress:\\n(.*) "));
            rxmIP=rxIP.match(sHTML);
            if(rxmIP.hasCaptured(1))
                qDebug() << QStringLiteral("Name/IP: %1").arg(rxmIP.captured(1));
        }
    }
}

void BrowserWorker::setCooldown(uint uiNewCooldown) {
    uiCooldown=uiNewCooldown;
}

void BrowserWorker::setMode(RunMode rmNewMode) {
    rmMode=rmNewMode;
}
