/*
MIT License

Copyright (c) 2021 WagonWheelRobotics

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "aboutDialog.h"
#include "ui_aboutDialog.h"

#include "configStorage.h"

#include <QSslSocket>

#ifdef USE_PLOT_VIEW
#ifdef USE_QCUSTOMPLOT
#include "qcustomplot.h"
#endif
#endif

#ifdef CERES_EXISTS
#include "ceres/ceres.h"
#endif

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);

    //this->setWindowTitle(APP_NAME);

    ui->lb1->setText(APP_NAME " " APP_VERSION);
    ui->lb2->setText("(C) " COPYRIGHT_YEAR " " COMPANY_NAME " All rights reserved");

    QString info = "Third party libraries:\n";

    info += "Qt " APP_VERSION_QT;
    if(QSslSocket::supportsSsl())
    {
        info+="\n";
        info+=QSslSocket::sslLibraryBuildVersionString();
    }

#ifdef USE_MAP_VIEW
#ifdef USE_QGEOVIEW
    info += "\nQGeoView";
#endif
#endif

#ifdef USE_PLOT_VIEW
#ifdef USE_QCUSTOMPLOT
    info += "\nQCustomPlot " QCUSTOMPLOT_VERSION_STR;
#endif
#endif

#ifdef CERES_EXISTS
    info += "\nCeres solver " CERES_VERSION_STRING;
#endif


    ui->te->setText(info);
}

aboutDialog::~aboutDialog()
{
    delete ui;
}

void aboutDialog::on_pbExploreConfigFolder_clicked()
{
    configStorage c("", this);
    c.explore();
}
