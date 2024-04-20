#include "calibOptionsDialog.h"
#include "ui_calibOptionsDialog.h"

calibOptionsDialog::calibOptionsDialog(double t0, double t1, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::calibOptionsDialog)
{
    _t0 = t0;
    _t1 = t1;

    ui->setupUi(this);
    ui->leStart->setText(QString("%1").arg(t0,3,'f'));
    ui->leEnd->setText(QString("%1").arg(t1,3,'f'));
    _params.clear();
}

calibOptionsDialog::~calibOptionsDialog()
{
    delete ui;
}

void calibOptionsDialog::on_buttonBox_accepted()
{
    bool ok1,ok2;
    double t0=ui->leStart->text().toDouble(&ok1);
    double t1=ui->leEnd->text().toDouble(&ok2);
    if(ok1 && ok2)
    {
        QVariantMap p;
        p["t0"] = t0;
        p["t1"] = t1;
        if(t0<t1 && _t0<=t0 && t1<=_t1)
        {
            _params=p;
        }
    }
    QDialog::accept();
}

const QVariantMap &calibOptionsDialog::params() const
{
    return _params;
}


void calibOptionsDialog::on_buttonBox_rejected()
{
    QDialog::reject();
}

