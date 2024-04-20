#ifndef GL_PCLOUD_ENTITY_H
#define GL_PCLOUD_ENTITY_H

/*
Copyright 2021 Wagon Wheel Robotics

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "gl_entity_ctx.h"

#include <mutex>

#include <QVector>
#include <QMap>

typedef struct
{
    QOpenGLBuffer vbo;
    int n;
} vbo_t;

typedef struct
{
    quint64 total;
    quint64 remain;
    GLfloat *vertex;
    GLfloat *curTop;
    int mode;
    int counter;
} vbo_ctx_t;



typedef QVector<vbo_t> vvbo_t;

class gl_pcloud_entity : public gl_entity_ctx
{
    Q_OBJECT
public:

    explicit gl_pcloud_entity(QObject *parent = 0);
    virtual ~gl_pcloud_entity();

    static void init_opt_pc(opt_pointcloud_t &p);

    virtual void cleanup(void);

    GLfloat *vertex(void) {return _vertex;}
    quint64 nVertex(void) {return _nVertex;}
    int nElement(void) {return _nElement;}

    virtual void draw_gl(gl_draw_ctx_t &draw);
    virtual int update_draw_gl(gl_draw_ctx_t &draw);
    virtual int rebuildRequest(void);   //rebuild VBO

    virtual QVector3D getCenter(void);

    virtual int prepare_gl(void);  //called by opengl gui thread
    virtual int pertialPrepare_gl(void);
    virtual bool isUnloadable(void) {return true;}
    virtual bool isExportable(void) {return true;}

public slots:
    void load(void);        //data load thread

protected:
    virtual void vbo_bind(QOpenGLBuffer &vbo, QOpenGLFunctions *fc);
    virtual void vbo_release(QOpenGLBuffer &vbo,QOpenGLFunctions *fc);

    virtual int load_mem(const uint8_t *buf, size_t length);

private:
    void partialVBOallocation(void);

protected:
    static std::mutex _prgMutex;
    static int _prgCount;
    static QMap<int, QOpenGLShaderProgram*> _prg;

    vvbo_t _vvbo;
    vbo_ctx_t _vboCtx;

    GLfloat *_vertex;
    quint64 _nVertex;
    int _nElement;
    uint32_t _format;

    int _rgb;
    int _amp;
    int _rng;
    int _flg;

};

#endif // GL_PCLOUD_ENTITY_H
