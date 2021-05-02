#include "hintdialog.h"
#include "ui_hintdialog.h"

HintDialog::HintDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HintDialog)
{
    ui->setupUi(this);
}

HintDialog::~HintDialog()
{
    delete ui;
}
