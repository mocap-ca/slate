#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QString host, int port, int fontSize, bool isServer, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->lineEditHost->setText(host);
    ui->lineEditPort->setText(QString("%1").arg(port));
    ui->lineEditFont->setText(QString("%1").arg(fontSize));
    ui->radioButtonClient->setChecked(true);
    ui->radioButtonServer->setChecked(false);
    if(isServer)
    {
	setServer();
	ui->radioButtonServer->setChecked(true);
    }
    else
    {
	setClient();
        ui->radioButtonClient->setChecked(true);
    }
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
QString Dialog::getPort() {  return ui->lineEditPort->text(); }
QString Dialog::getFont() {  return ui->lineEditFont->text(); }
