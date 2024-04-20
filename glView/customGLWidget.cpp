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

#include "customGLWidget.h"
#include "viewOptionsDialog.h"

#include "gl_3axis_entity.h"
#include "gl_pcloud_entity.h"
#include "rot.h"

#ifdef USE_EDL
#include "ccFrameBufferObject.h"
#include "ccGlFilter.h"
#include "ccEDLFilter.h"
#include <cfloat>
#endif

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QThread>

static void applyViewOptions(const QVariantMap x, viewOptions &opts)
{
    opts.pointAntiAlias= x["cbPointAntiAlias"].toInt();
    opts.persFOV= x["dsbPersFOV"].toDouble();
    opts.persNear= x["dsbPersNear"].toDouble();
    opts.persFar= x["dsbPersFar"].toDouble();
    opts.orthNear= x["dsbOrthNear"].toDouble();
    opts.orthFar= x["dsbOrthFar"].toDouble();
}

void customGLWidget::viewOptionsTriggered(void)
{
    viewOptionsDialog dlg(_viewOptionsStorage,this);
    if(dlg.exec()==QDialog::Accepted)
    {
        _viewOptionsStorage=dlg._opts;
        viewOptionsDialog::save(_viewOptionsStorage);
        applyViewOptions(_viewOptionsStorage,_viewOptions);
        draftUpdate();
    }
}

customGLWidget::customGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    _viewOptionsStorage=viewOptionsDialog::load();
    applyViewOptions(_viewOptionsStorage,_viewOptions);

#ifdef USE_EDL
    m_fbo = nullptr;
    m_activeFbo = nullptr;
    m_activeGLFilter=nullptr;
#endif

    _depthContext=nullptr;
    _poiIndicator=nullptr;

    _cameraMode=CAM_PERSPECTIVE;
    _cameraControl=CAM_CTRL_LEGACY; //CAM_CTRL_POTTERSWHEEL;

    {
        _depthSearchRadius=24;
        int x=0,y=0;
        int step=1;
        int sign=1;
        _depthSearchArea<<QPoint(0,0);
        while(_depthSearchArea.size()<(_depthSearchRadius-1)*(_depthSearchRadius-1))
        {
            for(int j=0;j<step;j++)
            {
                y+=sign;
                _depthSearchArea<<QPoint(x,y);
            }
            for(int j=0;j<step;j++)
            {
                x+=sign;
                _depthSearchArea<<QPoint(x,y);
            }
            step++;
            sign*=-1;
        }
    }
}

customGLWidget::~customGLWidget()
{
#ifdef USE_EDL
    if(m_fbo!=nullptr)
    {
        delete m_fbo;
        m_fbo = nullptr;
    }

    if(m_activeGLFilter!=nullptr)
    {
        delete m_activeGLFilter;
        m_activeGLFilter = nullptr;
    }
#endif

    if(_depthContext!=nullptr)
    {
        delete _depthContext;
    }
}

QSize customGLWidget::sizeHint() const
{
    return QSize(1024,768);
}

void customGLWidget::load_stock(void)
{
    gl_3axis_entity *poiCtx=new gl_3axis_entity;
    poiCtx->origin= get_origin();
    poiCtx->setObjectName("POI");
    poiCtx->load();
    emit entityLoadedByWidget(poiCtx);

    _poiIndicator=poiCtx;

}

void customGLWidget::initializeGL()
{
    memset(&_draw,0,sizeof(gl_draw_ctx_t));
    gl_pcloud_entity::init_opt_pc(_draw.opt_pc);

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, [=]()
    {
        makeCurrent();

        doneCurrent();
    });

    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1);
    _draw.lightDir=QVector3D(0, 0, -1);
    _draw.draftOnly=false;
    _draw.eyeDomeLighting=1;
    _draw.pointAntiAlias=1;

    //initialize camera parameter (dcm, POI and range)
    QVector3D e(10.0*rot::d2r,20.0*rot::d2r,30.0*rot::d2r);	//initial attitude of camera : roll,pitch yaw
    rot::euler_to_quat(_quat, e);

    _range=5.0;
    _poi=QVector3D(0.0f,0.0f,0.0f);
    update_by_poi();

    QMatrix3x3 dcm;
    rot::dcm_from_quat(dcm,_quat);

    _pot[0]= -std::atan2(-dcm(0,2),dcm(2,2))*rot::r2d;
    _pot[1]= -std::atan2(dcm(1,0),dcm(1,1))*rot::r2d;

    _next_mode=GL_DRAW_NORMAL;

#ifdef USE_EDL
    initFBO(128,128);
    m_activeGLFilter=new ccEDLFilter;
    m_glExtFuncSupported = m_glExtFunc.initializeOpenGLFunctions();
#endif

    connect(&_timer4update, SIGNAL(timeout()), this, SLOT(update()));
    connect(&_timer4pertialPrepare, SIGNAL(timeout()), this, SLOT(pertialPrepare()));

    _timer4pertialPrepare.start(1000/60);

    load_stock();

    emit initialized();

    setMouseTracking(true);
}

void customGLWidget::resizeGL(int w, int h)
{
    if(h<1) h=1;

    _draw.width=w;
    _draw.height=h;
    _draw.aspectRatio=(double)(_draw.width) / (double)(_draw.height);

    if(_depthContext!=nullptr)
    {
        delete _depthContext;
    }
    _depthContext=new depthContext(_draw.width, _draw.height);

#ifdef USE_EDL
    if(m_fbo)
    {
        initFBO(w, h);
    }
    if (m_activeGLFilter)
    {
        initGLFilter(w, h, true);
    }
#endif
}

void customGLWidget::mousePressEvent(QMouseEvent *event)
{
    _mouseMoveLastPos = event->pos();

    if (event->button()==Qt::RightButton)
    {
        _RightPressedPos = event->pos();
    }
}

void customGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && !_RightPressedPos.isNull())
    {
        QPoint delta= event->pos() - _RightPressedPos;
        _RightPressedPos=QPoint();
        if(delta.manhattanLength()<5)
        {
        //    openContextMenu(event, QCursor::pos());
        }
    }
}

void customGLWidget::paintGL()
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    _timer4update.stop();

    int nextTimeout=500;

    p.endNativePainting();

    OpenGLFunctions* glfunc=functions();

#ifdef USE_EDL
    if(_draw.eyeDomeLighting)
    {
        bindFBO(m_fbo);
    }
    else
    {
        makeCurrent();
    }
#else
    makeCurrent();
#endif

    draw_core(_next_mode);


#ifdef USE_EDL
    if(_draw.eyeDomeLighting)
    {
        bindFBO(nullptr);
        GLuint depthTex = m_fbo->getDepthTexture();
        GLuint colorTex = m_fbo->getColorTexture();

        ccGlFilter::ViewportParameters parameters;
        {
            parameters.perspectiveMode = _cameraMode==CAM_PERSPECTIVE;
            parameters.zFar = parameters.perspectiveMode ? _viewOptions.persFar:_viewOptions.orthFar;
            parameters.zNear = parameters.perspectiveMode ? _viewOptions.persNear:_viewOptions.orthNear;
            parameters.zoom = parameters.perspectiveMode ? computePerspectiveZoom() : 3.0;//m_viewportParams.zoom; //TODO: doesn't work well with EDL in perspective mode!
        }

        m_activeGLFilter->shade(depthTex, colorTex, parameters);
        bindFBO(nullptr); //in case the active filter has used a FBOs!

        GLuint screenTex = m_activeGLFilter->getTexture();
        if(glfunc->glIsTexture(screenTex))
        {
            setStandardOrthoCorner();
            glfunc->glPushAttrib(GL_DEPTH_BUFFER_BIT);
            glfunc->glDisable(GL_DEPTH_TEST);

            ccGLUtils::DisplayTexture2DPosition(screenTex, 0, 0, _draw.width, _draw.height);

            glfunc->glBindTexture(GL_TEXTURE_2D, defaultFramebufferObject());
            glfunc->glPopAttrib(); //GL_DEPTH_BUFFER_BIT
        }
    }
    else
    {
        doneCurrent();
    }
#else
    doneCurrent();
#endif

    p.beginNativePainting();

    if(_next_mode==GL_DRAW_TEMP)
    {
        if(!_draw.draftOnly)
        {
            _next_mode=GL_DRAW_NORMAL;

            _timer4update.start(nextTimeout);
        }
    }
    else
    {

    }
}



void customGLWidget::draftUpdate(void)
{
    _next_mode=GL_DRAW_TEMP;
    emit update();
}


void customGLWidget::poiIndicatorUpdate(void)
{
    if(_poiIndicator!=nullptr)
    {
        _poiIndicator->local.setToIdentity();
        _poiIndicator->local.translate(_poi);
    }
}

void customGLWidget::draw_core(int mode)
{
    _draw.mode=mode;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);

    QMatrix3x3 dcm;
    rot::dcm_from_quat(dcm,_quat);

    GLfloat aspectRatio=GLfloat(_draw.width) / _draw.height;

    _draw.proj.setToIdentity();
    switch(_cameraMode)
    {
    case    CAM_PERSPECTIVE:
        _draw.proj.perspective(_viewOptions.persFOV, aspectRatio, _viewOptions.persNear,_viewOptions.persFar);
        break;

    case    CAM_ORTHO:
        _draw.proj.ortho(-_range,_range,-_range/aspectRatio,_range/aspectRatio,_viewOptions.orthNear,_viewOptions.orthFar);
        break;
    }

    _draw.pointAntiAlias=_viewOptions.pointAntiAlias;

    _draw.eyeDir=_poi-_poc;

    _draw.camera.setToIdentity();
    _draw.camera.lookAt(_poc,_poi,QVector3D(dcm(0,2),dcm(1,2),dcm(2,2)));

    _draw.world.setToIdentity();

    lockEntities();

    foreach(auto ctx,_entities.values())
    {
        if(!ctx->isAlphaBlend() && ctx->isPickable() && !ctx->isReference())
        {
            ctx->draw_gl(_draw);
        }
    }


    {   //read depth buffer
        glReadPixels(0,0,_draw.width,_draw.height,GL_DEPTH_COMPONENT,GL_FLOAT,_depthContext->depth());
        GLenum error=glGetError();
        if(!error)
        {
            _depthContext->setWhich(1);
            _altDepthRequired=0;
        }
        else
        {
            _altDepthRequired=1;
            updateDepth();
        }
    }

    foreach(auto ctx,_entities.values())
    {
        if(!ctx->isAlphaBlend() && !ctx->isPickable() && !ctx->isReference())
        {
            ctx->draw_gl(_draw);
        }
    }

    if(mode!=GL_DRAW_PICK)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    foreach(auto ctx,_entities.values())
    {
        if(ctx->isAlphaBlend() && !ctx->isReference())
        {
            ctx->draw_gl(_draw);
        }
    }

    if(mode!=GL_DRAW_PICK)
    {
        glDisable(GL_BLEND);
    }

    glGetIntegerv(GL_VIEWPORT, _draw.viewport);
    unlockEntities();

}

//--------------------------------------------------------------------------------
// Depth buffer for Picking
//--------------------------------------------------------------------------------

void customGLWidget::updateDepth(void)
{
    if(_altDepthRequired)
    {
        qDebug()<<"altDepth";
        makeCurrent();
        {
            qt_opengl_depth depth(_depthContext);     //FBO is prepared in this constructor
            draw_core(GL_DRAW_PICK);
            depth.read();
        }
        doneCurrent();
    }
}

int customGLWidget::unproj(int winX,int winY, QVector3D &ret)
{
    int x,y,valid=0;
    GLfloat z;
    x=winX;
    y=_draw.height-winY;

    int r=_depthSearchRadius/2+1;

    if((r<x) && (x+r<_draw.width) &&
       (r<y) && (y+r<_draw.height))
    {
        if(_depthContext!=nullptr)
        {
            foreach(auto p,_depthSearchArea)
            {
                if(_depthContext->get_depth(x+p.x(),y+p.y(),z))
                {
                    valid=qt_opengl_unproj(QVector3D(x+p.x(),y+p.y(),z), _draw.proj * _draw.camera * _draw.world, _draw.viewport, ret);
                    if(valid)
                    {
                        break;
                    }
                }
            }
        }
    }
    return valid;
}

//--------------------------------------------------------------------------------
// User interface
//--------------------------------------------------------------------------------

void customGLWidget::onCmPerspectiveView(bool checked)
{
    Q_UNUSED(checked)
    _cameraMode=CAM_PERSPECTIVE;
    leftAxisInput(0.0,0.0);
    update_by_poi();
    draftUpdate();
}

void customGLWidget::onCmOrthoView(bool checked)
{
    Q_UNUSED(checked)
    _cameraMode=CAM_ORTHO;
    leftAxisInput(0.0,0.0);
    update_by_poi();
    draftUpdate();
}

void customGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(_mouseMoveLastPos.isNull()) _mouseMoveLastPos=event->pos();
    double dx = (event->x() - _mouseMoveLastPos.x()) / (double)_draw.width;
    double dy = (event->y() - _mouseMoveLastPos.y()) / (double)_draw.height;
    if( QApplication::keyboardModifiers() & Qt::ControlModifier ){  dx=0.0; }
    if( QApplication::keyboardModifiers() & Qt::AltModifier )    {  dy=0.0; }

    if (event->buttons() & Qt::LeftButton)
    {
        if(leftAxisInput(dx,dy))
        {
            update_by_poi();
            draftUpdate();
        }
    }
    else if (event->buttons() & Qt::RightButton)
    {
        _RightPressedPos=QPoint();
        if(rightAxisInput(dx,dy))
        {
            update_by_poi();
            draftUpdate();
        }
    }
    else
    {
        bool updateRequired=false;
        QVector3D p;
        if( unproj( event->x(), event->y(), p) )
        {
        }
        if(updateRequired)
        {
            update();
        }
    }

    _mouseMoveLastPos = event->pos();
}

void customGLWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(!getEntitiesCount()) return;

    if( unproj( event->x(), event->y(), _poi) )
    {
        QStringList poiStrLst;
        poiStrLst << QString::asprintf("%.3f",_poi.x());
        poiStrLst << QString::asprintf("%.3f",_poi.y());
        poiStrLst << QString::asprintf("%.3f",_poi.z());
        emit poiUpdated(poiStrLst);

        update_by_poi();    //move camera location
        draftUpdate();
    }
}

void customGLWidget::keyPressEvent(QKeyEvent *event)
{
    qDebug()<<"key"<<event->key();
    switch(event->key())
    {
    case    Qt::Key_F9:
    case    Qt::Key_F10:
    case    Qt::Key_F11:
    case    Qt::Key_F12:
        //emit keyPressFromGLWidget(event->key());
        break;
    case    Qt::Key_F5: if(translateInput(0.0,0.0, 0.5)){update_by_poi();draftUpdate();} break;
    case    Qt::Key_F6: if(translateInput(0.0,0.0,-0.5)){update_by_poi();draftUpdate();} break;
    case    Qt::Key_F3: if(zoomInput(-0.15)){update_by_poi();draftUpdate();} break;
    case    Qt::Key_F4: if(zoomInput( 0.15)){update_by_poi();draftUpdate();} break;
    case    Qt::Key_P:  _cameraControl = CAM_CTRL_POTTERSWHEEL; break;
    case    Qt::Key_T:  _cameraControl = CAM_CTRL_LEGACY; break;
    case    Qt::Key_L:  break;
    case    Qt::Key_O:  break;
    case    Qt::Key_D:  break;

    case    Qt::Key_PageDown: if(cameraLookingDown()){update_by_poi();draftUpdate();}   break;
    case    Qt::Key_Z:  break;
    case    Qt::Key_A:  break;
    case    Qt::Key_M:  break;

    case    Qt::Key_Escape:  break;
    case    Qt::Key_1:  onCmPerspectiveView(true);    break;
    case    Qt::Key_2:  onCmOrthoView(true);          break;
    case    Qt::Key_3:     break;
    case    Qt::Key_Slash: break;
    }
}

void customGLWidget::wheelEvent(QWheelEvent *event)
{
    QPoint d = event->angleDelta();
    if(zoomInput(-d.y()*0.01/8.0))
    {
        update_by_poi();
        draftUpdate();
    }
}

//--------------------------------------------------------------------------------
// Camera control User Interface functions
//--------------------------------------------------------------------------------

int customGLWidget::trackballInput(double dx,double dy,double dz)
{
    float gain=100.0f;
    rot::rot(-gain*dz, gain*dy,-gain*dx,_quat);
    return 1;
}

int customGLWidget::pottersWheelInput(double dx,double dy)
{
    //potter's wheel mode
    QVector4D qTilt,qYaw;
    QMatrix3x3 dcm,dcmT,dcmY;
    const QVector3D Yaxis(0.0,-1.0,0.0);  //Y axis
    const QVector3D Zaxis(0.0,0.0,1.0);   //Z axis
    float gain=150.0f;

    _pot[0]=rot::normalize180(_pot[0]+gain*dy);  //tilt
    _pot[1]=rot::normalize180(_pot[1]+gain*dx);  //yaw

    rot::quat_from_axis_angle(qTilt,Yaxis,_pot[0]*rot::d2r);
    rot::dcm_from_quat(dcmT,qTilt);    //tilt rotation matrix

    rot::quat_from_axis_angle(qYaw,Zaxis,_pot[1]*rot::d2r);
    rot::dcm_from_quat(dcmY,qYaw);    //yaw rotation matrix

    dcm = dcmT*dcmY;
    rot::dcm_to_quat(_quat,dcm);

    return 1;
}

int customGLWidget::cameraLookingDown()
{
    if(_cameraControl==CAM_CTRL_LEGACY)
    {
        _pot[1]=0.0;
    }
    _pot[0]=90.0;

    return pottersWheelInput(0.0,0.0);
}

int customGLWidget::zoomInput(double dx)
{
    _range +=dx*_range;
    if(_range<1.0) _range=1.0;
    if(_range>10000.0) _range=10000.0;
    return 1;
}

int customGLWidget::translateInput(double dx, double dy, double dz)
{
    QMatrix3x3 dcm;
    rot::dcm_from_quat(dcm,_quat);
    _poi+=QVector3D(dcm(0,1), dcm(1,1), dcm(2,1) )*_range*dx*2;
    _poi+=QVector3D(dcm(0,2), dcm(1,2), dcm(2,2) )*_range*dy*2/_draw.aspectRatio;
    _poi+=QVector3D(dcm(0,0), dcm(1,0), dcm(2,0) )*dz;
    return 1;
}

int customGLWidget::leftAxisInput(double dx, double dy)
{
    int ret=0;
    if(_cameraControl==CAM_CTRL_LEGACY)
    {
        ret=trackballInput(dx,dy,0.0);
    }
    else if(_cameraControl==CAM_CTRL_POTTERSWHEEL)
    {
        ret=pottersWheelInput(dx,dy);
    }
    return ret;
}

int customGLWidget::rightAxisInput(double dx, double dy)
{
    int ret=0;
    if(_cameraControl==CAM_CTRL_LEGACY)
    {
        ret=trackballInput(0.0,0.0,dx);
        ret+=zoomInput(dy);
    }
    else if(_cameraControl==CAM_CTRL_POTTERSWHEEL)
    {
        ret=translateInput(dx,dy,0.0);
    }
    return ret;
}

void customGLWidget::update_by_poi(void)//update poc by quat, poi and range
{
    QMatrix3x3 dcm;
    float setBack=1.0f;
    rot::dcm_from_quat(dcm,_quat);

    setBack=_range;

    _poc=QVector3D(_poi.x()-dcm(0,0)*setBack, _poi.y()-dcm(1,0)*setBack, _poi.z()-dcm(2,0)*setBack);

    poiIndicatorUpdate();
}

//--------------------------------------------------------------------------------
// Entities Access Control
//--------------------------------------------------------------------------------

void customGLWidget::lockEntities(void)
{
    _mtxEntities.lock();
}
void customGLWidget::unlockEntities(void)
{
    _mtxEntities.unlock();
}

void customGLWidget::delayLoad(gl_entity_ctx *ctx, const char *path)
{
    ctx->origin = get_origin();
    ctx->info[ENTITY_INFO_TARGET_FILENAME] = QString(path);

    QThread *x=new QThread;
    ctx->moveToThread(x);
    x->start();

    connect(ctx, &gl_entity_ctx::done, this, [=](QObject *x)
    {
        emit entityLoadedByWidget(x);
    });

    QMetaObject::invokeMethod(ctx, "load");
}

void customGLWidget::delayLoad(gl_entity_ctx *ctx, const QByteArray &data)
{
    ctx->origin = get_origin();
    ctx->info[ENTITY_INFO_TARGET_BYTES] = data;

    QThread *x=new QThread;
    ctx->moveToThread(x);
    x->start();

    connect(ctx, &gl_entity_ctx::done, this, [=](QObject *x)
    {
        emit entityLoadedByWidget(x);
    });

    QMetaObject::invokeMethod(ctx, "load");
}


void customGLWidget::pertialPrepare(void)
{
    bool redraw=false;
    if(_entitiesNotCompleted.size())
    {
        lockEntities();
        makeCurrent();
        foreach(auto key,_entitiesNotCompleted.keys())
        {
            if(!_entitiesNotCompleted[key]->pertialPrepare_gl())
            {
                _entitiesNotCompleted.remove(key);
                if(!_entitiesNotCompleted.size())
                {//final one
                    redraw=true;
                }
            }
        }
        doneCurrent();
        unlockEntities();
    }
    if(redraw) draftUpdate();
}

void customGLWidget::rebuildRequest(QUuid id)
{
    lockEntities();
    if(_entities.contains(id))
    {
        if(_entities[id]->rebuildRequest())
        {
           _entitiesNotCompleted[ id ]=_entities[id];
        }
    }
    unlockEntities();
}

void customGLWidget::entityLoaded(QObject *x)
{
    gl_entity_ctx *ctx=dynamic_cast<gl_entity_ctx*>(x);
    if(ctx!=NULL)
    {
        if(ctx->is_valid())
        {
            qDebug() << "target:" << ctx->info[ENTITY_INFO_TARGET_FILENAME].toString();

            qDebug() << "prepare begin" << thread();
            lockEntities();
            makeCurrent();
            if(ctx->prepare_gl())
            {
                _entitiesNotCompleted[ ctx->uniqueId() ]=ctx;
            }
            doneCurrent();
            _entities[ ctx->uniqueId() ]=ctx;
            unlockEntities();
            qDebug() << "gl_entity_ctx prepared"  << thread();

            if(ctx->update_draw_gl(_draw))
            {   //update parameter to control dialog
                emit onDrawingOptionUpdated();
            }

            draftUpdate();
        }
        else
        {
            qDebug() << "invalid gl_entity_ctx";
            //delete ctx;
        }
    }
}

void customGLWidget::entityUnload(QObject *x)
{
    gl_entity_ctx *ctx=dynamic_cast<gl_entity_ctx*>(x);
    if(ctx!=NULL)
    {
        lockEntities();
        if(_entities.contains(ctx->uniqueId()))
        {
            _entities.remove(ctx->uniqueId());
        }
        unlockEntities();

        emit entityUnloaded(x);

        qDebug()<<ctx->getCaption()<<"unloaded.";

        ctx->cleanup();

        ctx->deleteLater();

        draftUpdate();
    }
}

void customGLWidget::redrawEntity(void)
{
    draftUpdate();
}

void customGLWidget::entityClicked(gl_entity_ctx *a)
{
    if(a->setMasterOriginFromLocal(a->getCenter()))
    {
        _poi=QVector3D(0.0,0.0,0.0);

        update_by_poi();    //move camera location
        draftUpdate();
    }
}

size_t customGLWidget::getEntitiesCount(void)
{
    lockEntities();
    size_t count=_entities.size();
    unlockEntities();
    return count;
}


#ifdef USE_EDL
bool customGLWidget::initFBOSafe(ccFrameBufferObject* &fbo, int w, int h)
{
    //correction for HD screens
    const int retinaScale = devicePixelRatio();
    w *= retinaScale;
    h *= retinaScale;

    if (fbo && (fbo->width() == (unsigned)w) && (fbo->height() == (unsigned)h))
    {
        //nothing to do
        return true;
    }

    //we "disconnect" the current FBO to avoid wrong display/errors
    //if QT tries to redraw window during initialization
    ccFrameBufferObject* _fbo = fbo;
    fbo = nullptr;

    if (!_fbo)
    {
        _fbo = new ccFrameBufferObject();
    }

    if (!_fbo->init(w, h)
        || !_fbo->initColor()
        || !_fbo->initDepth())
    {
        delete _fbo;
        _fbo = nullptr;
        return false;
    }

    fbo = _fbo;
    return true;
}
void customGLWidget::removeFBOSafe(ccFrameBufferObject* &fbo)
{
    //we "disconnect" the current FBO to avoid wrong display/errors
    //if QT tries to redraw window during object destruction
    if (fbo)
    {
        ccFrameBufferObject* _fbo = fbo;
        fbo = nullptr;
        delete _fbo;
    }
}
void customGLWidget::removeFBO()
{
    removeFBOSafe(m_fbo);
}
bool customGLWidget::initFBO(int w, int h)
{

    makeCurrent();

    if (!initFBOSafe(m_fbo, w, h))
    {
        return false;
    }

//	deprecate3DLayer();
    return true;
}


void customGLWidget::removeGLFilter()
{
    //we "disconnect" current glFilter, to avoid wrong display/errors
    //if QT tries to redraw window during object destruction
    ccGlFilter* _filter = nullptr;
    std::swap(_filter, m_activeGLFilter);

    if (_filter)
    {
        delete _filter;
        _filter = nullptr;
    }
}

bool customGLWidget::initGLFilter(int w, int h, bool silent/*=false*/)
{
    if (!m_activeGLFilter)
    {
        return false;
    }

    makeCurrent();

    //correction for HD screens
    const int retinaScale = devicePixelRatio();
    w *= retinaScale;
    h *= retinaScale;

    //we "disconnect" current glFilter, to avoid wrong display/errors
    //if QT tries to redraw window during initialization
    ccGlFilter* _filter = nullptr;
    std::swap(_filter, m_activeGLFilter);

    QString error;
    if (!_filter->init(static_cast<unsigned>(w), static_cast<unsigned>(h), ":/shaders", error))
    {
        if (!silent)
        {
            qWarning()<< QString("[GL Filter] Initialization failed: ") + error.trimmed();
        }
        return false;
    }

    if (!silent)
    {
        qWarning()<< "[GL Filter] Filter initialized";
    }

    m_activeGLFilter = _filter;

    return true;
}

bool customGLWidget::bindFBO(ccFrameBufferObject* fbo)
{
    if (fbo) //bind
    {
        if (fbo->start())
        {
            m_activeFbo = fbo;
            return true;
        }
        else
        {
            //failed to start the FBO?!
            m_activeFbo = nullptr;
            return false;

        }
    }
    else //unbind
    {
        m_activeFbo = nullptr;

        assert(m_glExtFuncSupported);
        //we automatically enable the QOpenGLWidget's default FBO
        m_glExtFunc.glBindFramebuffer(GL_FRAMEBUFFER_EXT, defaultFramebufferObject());

        return true;
    }
}

void customGLWidget::setStandardOrthoCorner()
{
    OpenGLFunctions* glfunc=functions();

    glfunc->glMatrixMode(GL_PROJECTION);
    glfunc->glLoadIdentity();
    glfunc->glOrtho(0.0, _draw.width, 0.0, _draw.height, 0.0, 1.0);
    glfunc->glMatrixMode(GL_MODELVIEW);
    glfunc->glLoadIdentity();
}

constexpr double ZERO_TOLERANCE = static_cast<double>(FLT_EPSILON);
constexpr double CC_DEG_TO_RAD = (M_PI/180.0);
float customGLWidget::computePerspectiveZoom() const
{
    //DGM: in fact it can be useful to compute it even in ortho mode :)
    //if (!m_viewportParams.perspectiveView)
    //	return m_viewportParams.zoom;

    //we compute the zoom equivalent to the corresponding camera position (inverse of above calculus)
    float currentFov_deg = _viewOptions.persFOV;
    if (currentFov_deg < FLT_EPSILON)
        return 1.0f;

    //Camera center to pivot vector
    double zoomEquivalentDist = (_poc - _poi).length();
    if (zoomEquivalentDist < ZERO_TOLERANCE)
        return 1.0f;

    //float screenSize = std::min(m_glViewport.width(), m_glViewport.height()) * m_viewportParams.pixelSize; //see how pixelSize is computed!
    float screenSize = _draw.width *0.3803/* m_viewportParams.pixelSize*/; //see how pixelSize is computed!
    return screenSize / static_cast<float>(zoomEquivalentDist * 2.0 * std::tan(currentFov_deg / 2.0 * CC_DEG_TO_RAD));
}

void customGLWidget::makeCurrent()
{
#ifdef CC_GL_WINDOW_USE_QWINDOW
    if (m_context)
    {
        m_context->makeCurrent(this);
    }
#else
    QOpenGLWidget::makeCurrent();
#endif

    if (m_activeFbo)
    {
        m_activeFbo->start();
    }
}

#endif

