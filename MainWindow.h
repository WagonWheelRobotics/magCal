#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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


#include <QMainWindow>

#include <Qlist>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#ifdef USE_MAP_VIEW
class customMapView;
#endif

class customFloatingWindow;
class QMdiSubWindow;
class customGLWidget;
class customMdiSubWindow;
class gl_entity_ctx;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Q_INVOKABLE void attachToMdi(QObject *fltWindow);
    Q_INVOKABLE void detachFromMdi(QObject *mdiWindow);

public slots:
    void logMessage(int level,QString text);

protected:
#ifdef USE_IMAGE_VIEW
    void openImage(const QString &fileName);
#endif

#ifdef USE_3D_VIEW
    void create3DView(void);
#endif

#ifdef USE_MAP_VIEW
    void createMapView(void);
#endif

private slots:
    void on_actionConfigure_3D_View_triggered();

    void on_actionAbout_triggered();

    void on_action3D_View_triggered();

    void on_actionSerial_port_triggered();

    void on_actionTCP_Client_triggered();

#ifdef USE_MAP_VIEW
    void on_actionMap_View_triggered();
#endif

    void on_actionOpen_triggered();

    void on_actionExecute_triggered();

    void on_actionExport_triggered();

private:

#ifdef USE_PLOT_VIEW
#ifdef EXAMPLE_CODE_QCP
#ifdef EXAMPLE_CODE_QCP_STATIC_PLOT
    void create_qcp_example_static();
#else
    double _qcp_example_t;
    void create_qcp_example_realtime();
#endif
#endif
#endif

    void loaded(QList<QList<double> > &dataSet);
    int solve(double t0, double t1, QVector<double> &k, int verbose=1);
    void plotCor(QVector<double> &k);

private:
    Ui::MainWindow *ui;

#ifdef USE_3D_VIEW
    customGLWidget *_glWidget;
    gl_entity_ctx *_stockModelPending;
    gl_entity_ctx *_stockModel;
#endif

    QList<customFloatingWindow*> _floating;
    QList<customMdiSubWindow*> _mdi;

#ifdef USE_MAP_VIEW
    customMapView *_mapWidget;
#endif

    QList<QList<double> > _norDataSet;  // normalized (time,x,y,z,w)
    QList<QList<double> > _scaDataSet;  // scaled (time,x,y,z,w)
    QList<QList<double> > _corDataSet;  // corrected (time,x,y,z,w)
    QVector<double> _k;
};
#endif // MAINWINDOW_H
