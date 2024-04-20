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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QMdiSubWindow>
#include <QStandardPaths>
#include <QSettings>
#include <QFileInfo>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QTimer>
#include <QImage>
#include <QFileDialog>

#include <QVector>
#include <QVariantList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>

#include "configStorage.h"

#include "customFloatingWindow.h"
#include "customMdiSubWindow.h"

#include <cmath>

#ifdef USE_MAP_VIEW
#include "customMapView.h"
#endif

#ifdef USE_IMAGE_VIEW
#include "customImageWidget.h"
#endif

#ifdef USE_3D_VIEW
#include "customGLWidget.h"
#include "gl_model_entity.h"
#include "gl_poses_entity.h"
#include "gl_pcloud_entity.h"
#include "gl_polyline_entity.h"

class gl_mag_entity : public gl_pcloud_entity
{
public:
    explicit gl_mag_entity(const QString &name, const QList<QList<double> > &data, QObject *parent = 0) : gl_pcloud_entity(parent)
    {
        _data = data;
        _name = name;
    }
    virtual ~gl_mag_entity()
    {

    }
    virtual void draw_gl(gl_draw_ctx_t &draw)
    {
        draw.opt_pc.color_mode = OPT_PC_CM_AMP;
        draw.opt_pc.psz=5.0f;
        draw.opt_pc.flt_amp[0]=0.0f;
        draw.opt_pc.flt_amp[1]=255.0f;
        gl_pcloud_entity::draw_gl(draw);
    }
protected:
    virtual int load_mem(const uint8_t *buf, size_t length)
    {
        _localOrigin = QVector3D(0.0f, 0.0f, 0.0f);
        setObjectName(_name);

        quint64 nPoints = _data.size();

        _rgb = 0;
        _amp = 3;
        _rng = 4;
        _flg = 5;

        _format = 0;
        _nElement = 6;

        _nVertex = nPoints;
        _vertex = new GLfloat [_nElement*_nVertex];

        float radius;
        //const float d2r = M_PI/180.0;

        GLfloat *w=_vertex;
        for(quint64 i=0; i<nPoints; i++)
        {
            const auto &v=_data.at(i);

            float radius = 1.0f;
            float x = v.at(1);
            float y = v.at(2);
            float z = v.at(3);

            float t=std::sqrt(x*x + y*y + z*z);

            float amp;
            float rng = t;

            if(t<0.95f) amp=10.0f;
            else if(t<1.05f) amp=0.5f*255.0f;
            else  amp=255.0f-10.0f;

            *w++ = x*radius;
            *w++ = y*radius;
            *w++ = z*radius;

            *w++ = amp;
            *w++ = rng;

            *w++ = 0.0f;    //flag
        }

        return _nVertex;
    }

private:
    QString _name;
    QList<QList<double> > _data;
};

#endif

#ifdef USE_PLOT_VIEW
#include "qcpPlotView.h"
#endif

static QString lastPath(QString name)
{
    configStorage s("path",nullptr);
    auto p=s.load("last");
    if(p.contains(name)) return p[name].toString();
    return QString();
}

static void setLastPath(QString name,QString fileName)
{
    configStorage s("path",nullptr);
    auto p=s.load("last");
    QFileInfo fi(fileName);
    p[name] = fi.absolutePath();
    s.save(p,"last");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _glWidget = nullptr;
    _stockModelPending = nullptr;
    _stockModel = nullptr;

    ui->peLog->setCenterOnScroll(true);
    //ui->mdiArea->tileSubWindows();

    setWindowTitle(APP_NAME);

#ifdef USE_IMAGE_VIEW
#ifdef EXAMPLE_CODE_IMAGE
    openImage(EXAMPLE_CODE_IMAGE);
#endif
#endif

#ifdef USE_PLOT_VIEW
#ifdef EXAMPLE_CODE_QCP
#ifdef EXAMPLE_CODE_QCP_STATIC_PLOT
    create_qcp_example_static();
#else
    create_qcp_example_realtime();
#endif
#endif
#endif

    create3DView();

#ifdef USE_MAP_VIEW
    createMapView();
#endif

    connect(ui->menuView, &QMenu::aboutToShow, this, [=](){
        ui->action3D_View->setChecked(_glWidget->parentWidget()->isVisible());
#ifdef USE_MAP_VIEW
        ui->actionMap_View->setChecked(_mapWidget->parentWidget()->isVisible());
#endif
    });

    connect(ui->menuFile, &QMenu::aboutToShow, this, [=](){
        ui->actionExecute->setEnabled(_norDataSet.size()>0);
        ui->actionExport->setEnabled(_k.size()>0);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

#ifdef USE_IMAGE_VIEW
void MainWindow::openImage(const QString &fileName)
{
    QImage image(fileName);
    if(!image.isNull())
    {
        QFileInfo fi(fileName);
        auto img=new customImageWidget(this);
        auto sub=new customMdiSubWindow(img, this);
        ui->mdiArea->addSubWindow(sub);
        img->setImage(image);
        sub->setWindowTitle(fi.baseName());
        sub->show();
        _mdi.append(sub);
    }
}
#endif

void MainWindow::create3DView()
{
    if(_glWidget==nullptr)
    {
        _glWidget = new customGLWidget(this);
        _glWidget->setProperty("DETACHABLE",false);
        auto sub=new customMdiSubWindow(_glWidget, this);
        ui->mdiArea->addSubWindow(sub);
        sub->setWindowTitle("3D View");
        sub->show();

        ui->tree->link(_glWidget);

        connect(_glWidget, &customGLWidget::initialized,[=]()
        {
            _stockModelPending = new gl_model_entity;
            _stockModelPending->setObjectName("unit sphere");
            //_stockModelPending->setReference(true);
            connect(_glWidget, &customGLWidget::entityLoadedByWidget,[=](QObject*o)
            {
                if(qobject_cast<gl_entity_ctx*>(o) == _stockModelPending)
                {
                    _stockModel = _stockModelPending;
#ifdef EXAMPLE_CODE_3D
                    //test code
                    if(_stockModel!=nullptr)
                    {
                        QByteArray dummy;
                        dummy.append((uint8_t)0x00);
                        _glWidget->delayLoad(new gl_poses_entity(_stockModel), dummy);
                        dummy[0]=1;
                        _glWidget->delayLoad(new gl_poses_entity(_stockModel), dummy);
                    }

                    {
                        QByteArray dummy;
                        dummy.append(1);
                        _glWidget->delayLoad(new gl_polyline_entity, dummy);
                        _glWidget->delayLoad(new gl_pcloud_entity, dummy);
                    }
#endif
                }
            });
            _glWidget->delayLoad(_stockModelPending, ":/gl/models/sphere.mqo");
        });
    }
}

#ifdef USE_MAP_VIEW
void MainWindow::createMapView()
{
    _mapWidget = new customMapView(this);
    _mapWidget->layout()->setContentsMargins(0,0,0,0);
    customMdiSubWindow *sub = new customMdiSubWindow(_mapWidget, this);
    sub->setWindowTitle("Map View");
    ui->mdiArea->addSubWindow(sub);
}
#endif

void MainWindow::attachToMdi(QObject *fltWindow)
{
    customFloatingWindow *w = qobject_cast<customFloatingWindow*>(fltWindow);
    if(w!=nullptr)
    {
        QSize sz = w->widget()->size();
        customMdiSubWindow *sub = new customMdiSubWindow(w->takeWidget(), this);
        sub->setWindowTitle(w->windowTitle());

        if(w->property("CLOSABLE").isValid()) sub->setProperty("CLOSABLE", w->property("CLOSABLE"));

        _floating.removeAll(w);

        ui->mdiArea->addSubWindow(sub);

        w->deleteLater();

        sub->resize(sz);
        sub->show();
    }
}

void MainWindow::detachFromMdi(QObject *mdiWindow)
{
    QMdiSubWindow *w=qobject_cast<QMdiSubWindow*>(mdiWindow);
    if(w!=nullptr)
    {
        QPoint global=w->mapToGlobal(w->pos());
        QSize sz = w->widget()->size();
        customFloatingWindow *floating=new customFloatingWindow(w->widget(), this);
        floating->setWindowTitle(w->windowTitle());
        if(w->property("CLOSABLE").isValid()) floating->setProperty("CLOSABLE", w->property("CLOSABLE"));

        _floating.append(floating);

        ui->mdiArea->removeSubWindow(w);
        w->deleteLater();

        floating->move(global);
        //floating->move(this->mapFromGlobal(global));
        floating->resize(sz);
        floating->show();
        qDebug()<<floating->pos();
    }
}

void MainWindow::logMessage(int level,QString text)
{
    if(level<0)
    {
        ui->peLog->appendHtml(text);
    }
    else
    {
        ui->statusbar->showMessage(text,5000);
//      ui->peLog->appendPlainText(text);
    }
}


void MainWindow::on_actionConfigure_3D_View_triggered()
{
    _glWidget->viewOptionsTriggered();
}

#include "aboutDialog.h"
void MainWindow::on_actionAbout_triggered()
{
    aboutDialog dlg;
    dlg.exec();    
}


void MainWindow::on_action3D_View_triggered()
{
    if(_glWidget->parentWidget()->isHidden()) _glWidget->parentWidget()->show(); else _glWidget->parentWidget()->hide();
}

#ifdef USE_MAP_VIEW
void MainWindow::on_actionMap_View_triggered()
{
    if(_mapWidget->parentWidget()->isHidden()) _mapWidget->parentWidget()->show(); else _mapWidget->parentWidget()->hide();
}
#endif

#ifdef USE_PLOT_VIEW
#ifdef EXAMPLE_CODE_QCP
#ifdef EXAMPLE_CODE_QCP_STATIC_PLOT
void MainWindow::create_qcp_example_static()
{
    QVariantMap  m;

    QStringList header;
    header << "X" << "Y";
    m["headers"] = header;
    QVector<double> x,y;
    for(double i=0.0;i<2.0*M_PI;i+=0.1*M_PI)
    {
        x.append(i);
        y.append(std::sin(i));
    }
    QVariantList columns;
    columns.append(QVariant::fromValue(x));
    columns.append(QVariant::fromValue(y));
    m["columns"] = columns;
    m["realtime"] = false;

    auto p=new qcpPlotView(m, this);
    auto sub=new customMdiSubWindow(p->widget(), this);
    ui->mdiArea->addSubWindow(sub);
    sub->setWindowTitle("Plot View");
    sub->show();
}
#else
void MainWindow::create_qcp_example_realtime()
{
    _qcp_example_t = 0.0;

    QVariantMap  m;

    QStringList header;
    header << "X" << "Y1" << "Y2";
    m["headers"] = header;
    m["realtime"] = true;

    auto p=new qcpPlotView(m, this);
    auto sub=new customMdiSubWindow(p->widget(), this);
    ui->mdiArea->addSubWindow(sub);
    sub->setWindowTitle("Plot View");
    sub->show();

    QTimer *tqcp=new QTimer(this);
    connect(tqcp,&QTimer::timeout, this, [=]()
    {
        QVector<double> data;
        data.append(_qcp_example_t); //x axis
        data.append(std::sin(2.0*3.14159*_qcp_example_t/1.0)); //y axis
        if(!(int)(std::fmod(_qcp_example_t,10.0)*100.0)) data.append(std::nan("")); //append nan for no data
        else  data.append(std::cos(2.0*3.14159*_qcp_example_t/1.0)); //y axis
        p->addData(data);
        _qcp_example_t += 0.05; //50msec
    });
    tqcp->setInterval(50);
    tqcp->start();
}
#endif
#endif
#endif


#include "serialPortDialog.h"
#include <QSerialPort>
void MainWindow::on_actionSerial_port_triggered()
{
    serialPortDialog dlg(this);
    if(dlg.exec()==QDialog::Accepted)
    {
        auto port = dlg.get(this);
        if(port->open(QIODevice::ReadWrite))
        {
            qDebug()<< "Serial port is opened" << port->portName();


            port->close();  //this is just example code
        }
        else
        {
            qDebug()<< "Serial port open failed" << port->portName();
        }
    }
}

#include "tcpClientDialog.h"

void MainWindow::on_actionTCP_Client_triggered()
{
    tcpClientDialog dlg(this);
    if(dlg.exec()==QDialog::Accepted)
    {

    }
}

QList<double> parse(const QStringList &tokens)
{
    QList<double> ret;
    for(const auto &i:tokens)
    {
        bool ok;
        auto d=i.toDouble(&ok);
        if(!ok) return QList<double>();
        ret.append(d);
    }
    return ret;
}



void MainWindow::on_actionOpen_triggered()
{
    auto fileName=QFileDialog::getOpenFileName(this,"Select a log file",lastPath("mag"),"Magnetometer log file (*.txt;*.csv)");
    if(!fileName.isEmpty())
    {
        setLastPath("mag",fileName);

        QFile f(fileName);
        if(f.open(QFile::ReadOnly|QFile::Text))
        {
            QList<QList<double> > dataset;
            for(;;)
            {
                bool lineOk=false;
                auto bytes=f.readLine();
                if(bytes.isEmpty()) break;

                QString line=QString::fromLocal8Bit(bytes);
                line.remove("\n");
                auto tokens1 = line.split(',');
                auto tokens2 = line.split(' ');
                auto data1 = parse(tokens1);
                auto data2 = parse(tokens2);
                if(!data1.empty()||!data2.empty())
                {
                    auto &data = data1.size()>data2.size() ? data1 : data2;
                    if(data.size()>=3)
                    {
                        if(dataset.size())
                        {
                            if(dataset.back().size() == data.size())
                            {
                                dataset.append(data);
                                lineOk = true;
                            }
                        }
                        else
                        {
                            dataset.append(data);
                            lineOk = true;
                        }
                    }
                }
                /*if(lineOk)
                {
                    qDebug()<<dataset.back();
                }*/

            }
            f.close();

            if(dataset.size()>50)
            {
                loaded(dataset);
            }
            else
            {
                qWarning()<<"Not enough data";
            }

        }


    }
}

void MainWindow::loaded(QList<QList<double> > &dataSet)
{
    _norDataSet.clear();
    {
        QVariantMap  m;

        QStringList header;
        header << "Time" << "raw X" << "raw Y" << "raw Z" << "total";
        m["headers"] = header;

        QVector<double> t,x,y,z,w;
        double time=-1.0, sum=0.0;
        for(const auto &i:dataSet)
        {
            if(i.size()==3)
            {
                time +=1.0;
                t.append(time);
                x.append(i.at(0));
                y.append(i.at(1));
                z.append(i.at(2));
            }
            else
            {
                if(time<0.0) time = i.at(0);
                t.append(i.at(0)-time);
                x.append(i.at(1));
                y.append(i.at(2));
                z.append(i.at(3));
            }
            QList<double> n;
            auto w_=std::sqrt(x.back()*x.back() + y.back()*y.back() + z.back()*z.back());
            n << t.back() << x.back() << y.back() << z.back();
            _norDataSet.append(n);
            sum += w_;
            w.append(w_);
        }

        if(t.size()>16)
        {
            double scale=t.size()/sum;
            qInfo()<<"Preliminary scale factor is" << scale;
            for(int i=0;i<t.size();i++)
            {
                QList<double> n;
                n << t.at(i) << x.at(i) * scale << y.at(i) * scale << z.at(i) * scale;
                _scaDataSet.append(n);
            }

            QVariantList columns;
            columns.append(QVariant::fromValue(t));
            columns.append(QVariant::fromValue(x));
            columns.append(QVariant::fromValue(y));
            columns.append(QVariant::fromValue(z));
            columns.append(QVariant::fromValue(w));
            m["columns"] = columns;
            m["realtime"] = false;

            auto p=new qcpPlotView(m, this);
            auto sub=new customMdiSubWindow(p->widget(), this);
            ui->mdiArea->addSubWindow(sub);
            sub->setWindowTitle("Raw Data");
            sub->show();

            {
                QByteArray dummy;
                dummy.append((char)0);
                _glWidget->delayLoad(new gl_mag_entity("raw data",_scaDataSet), dummy);
            }
        }
    }
}


int MainWindow::solve(double t0, double t1, QVector<double> &k, int verbose)
{
    extern int solve(QList<QList<double> > &dataSet, double t0, double t1, QVector<double> &k);

    if(solve(_norDataSet,t0,t1, k))
    {
        if(verbose)
        {
            qInfo()<<"calibration parameters are solved";
            qInfo()<<"x"<<k[0]<<k[1]<<k[2];
            qInfo()<<"y"<<k[3]<<k[4]<<k[5];
            qInfo()<<"z"<<k[6]<<k[7]<<k[8];
        }
        _k = k;
        return 1;
    }
    return 0;
}

void MainWindow::plotCor(QVector<double> &k)
{
    QVariantMap  m;
    QStringList header;
    header << "Time" << "cor X" << "cor Y" << "cor Z" << "total";
    m["headers"] = header;
    _corDataSet.clear();
    QVector<double> t,x,y,z,w;
    double time=-1.0;
    for(const auto &i:_norDataSet)
    {
        if(time<0.0) time = i.at(0);
        t.append(i.at(0)-time);
        x.append(k[0]*i.at(1)*i.at(1) +k[1]*i.at(1) +k[2]);
        y.append(k[3]*i.at(2)*i.at(2) +k[4]*i.at(2) +k[5]);
        z.append(k[6]*i.at(3)*i.at(3) +k[7]*i.at(3) +k[8]);
        w.append(std::sqrt(x.back()*x.back() + y.back()*y.back() + z.back()*z.back()));

        QList<double> n;
        n <<t.back()<<x.back()<<y.back()<<z.back()<<w.back();
        _corDataSet.append(n);
    }

    QVariantList columns;
    columns.append(QVariant::fromValue(t));
    columns.append(QVariant::fromValue(x));
    columns.append(QVariant::fromValue(y));
    columns.append(QVariant::fromValue(z));
    columns.append(QVariant::fromValue(w));
    m["columns"] = columns;
    m["realtime"] = false;

    auto p=new qcpPlotView(m, this);
    auto sub=new customMdiSubWindow(p->widget(), this);
    ui->mdiArea->addSubWindow(sub);
    sub->setWindowTitle("Cor Data");
    sub->show();

    {
        QByteArray dummy;
        dummy.append(1);
        _glWidget->delayLoad(new gl_mag_entity("corrected data",_corDataSet), dummy);
    }
}

#include "calibOptionsDialog.h"

void MainWindow::on_actionExecute_triggered()
{
    calibOptionsDialog dlg(_norDataSet.front().front(), _norDataSet.back().front(), this);
    if(dlg.exec()==QDialog::Accepted)
    {
        auto p=dlg.params();
        if(p.contains("t0") && p.contains("t1"))
        {
            double t0=p["t0"].toDouble();
            double t1=p["t1"].toDouble();
            QVector<double> k;
            if(solve(t0,t1,k))
            {
                plotCor(k);
            }
        }
        else
        {
            qWarning()<<"Time range is wrong.";
        }
    }
    else
    {
        qInfo()<<"Canceled by user.";
    }
}


void MainWindow::on_actionExport_triggered()
{
    auto fileName=QFileDialog::getSaveFileName(this,"Select a Calibration file to export",lastPath("mag"),"JSON Files (*.json)");
    if(!fileName.isEmpty())
    {
        QFile f(fileName);
        if(f.open(QFile::WriteOnly|QFile::Text))
        {
            QTextStream t(&f);
            QVariantMap m;
            QVariantList x,y,z;
            x << _k[0] << _k[1] << _k[2];
            y << _k[3] << _k[4] << _k[5];
            z << _k[6] << _k[7] << _k[8];
            m["date"] = QDateTime::currentDateTimeUtc().toString();
            m["model"] = "cor = k[0] x raw^2 + k[1] x raw + k[2]";
            m["kx"] = x;
            m["ky"] = y;
            m["kz"] = z;
            QJsonDocument j(QJsonObject::fromVariantMap(m));
            t << j.toJson();
            f.close();
            qInfo()<<fileName <<"exported";
        }
    }
}

