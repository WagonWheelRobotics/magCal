#ifndef CALIBOPTIONSDIALOG_H
#define CALIBOPTIONSDIALOG_H

#include <QDialog>
#include <QVariantMap>

namespace Ui {
class calibOptionsDialog;
}

class calibOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit calibOptionsDialog(double t0, double t1,QWidget *parent = nullptr);
    ~calibOptionsDialog();

    const QVariantMap &params() const;

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::calibOptionsDialog *ui;
    double _t0;
    double _t1;
    QVariantMap _params;
};

#endif // CALIBOPTIONSDIALOG_H
