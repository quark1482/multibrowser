#include "multibrowser.h"

#include <QApplication>

int main(int argc,char *argv[]) {
    QApplication appMain(argc,argv);
    MultiBrowser mbMain;
    mbMain.show();
    return appMain.exec();
}
