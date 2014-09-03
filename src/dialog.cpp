#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->radioButtonClient->setChecked(true);
    ui->radioButtonServer->setChecked(false);
    connect(ui->radioButtonClient, SIGNAL(clicked()), this, SLOT(setClient()));
    connect(ui->radioButtonServer, SIGNAL(clicked()), this, SLOT(setServer()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::setClient()
{
    ui->radioButtonServer->setChecked(false);
    ui->lineEditHost->setEnabled(true);
}

void Dialog::setServer()
{
    ui->radioButtonClient->setChecked(false);
    ui->lineEditHost->setEnabled(false);
}

bool Dialog::isServer() { return ui->radioButtonServer->isChecked(); }
QString Dialog::getHost() {  return ui->lineEditHost->text(); }
QString Dialog::getPort() {  return ui->lineEditHost->text(); }
QString Dialog::getFont() {  return ui->lineEditHost->text(); }
