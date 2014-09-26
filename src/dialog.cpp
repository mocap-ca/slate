#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QString host, int port, int fontSizeSmall, int fontSizeBig, int fontSizeTimecode, eMode inMode, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->lineEditHost->setText(host);
    ui->lineEditPort->setText(QString("%1").arg(port));
    ui->lineEditFontSmall->setText(QString("%1").arg(fontSizeSmall));
    ui->lineEditFontBig->setText(QString("%1").arg(fontSizeBig));
    ui->lineEditFontTimecode->setText(QString("%1").arg(fontSizeTimecode));
    ui->radioButtonClient->setChecked(false);
    ui->radioButtonUDP->setChecked(false);
    ui->radioButtonNP->setChecked(true);

    if(inMode == Dialog::CLIENT )
    {
        setClient();
        ui->radioButtonClient->setChecked(true);
    }
    if(inMode == Dialog::UDP_SERVER )
    {
        setUDPServer();
        ui->radioButtonUDP->setChecked(true);
    }
    if(inMode == Dialog::NP_SERVER)
    {
        setNPServer();
        ui->radioButtonNP->setChecked(true);
    }

    connect(ui->radioButtonClient, SIGNAL(clicked()), this, SLOT(setClient()));
    connect(ui->radioButtonNP,     SIGNAL(clicked()), this, SLOT(setNPServer()));
    connect(ui->radioButtonUDP,    SIGNAL(clicked()), this, SLOT(setUDPServer()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::setClient()
{
    ui->radioButtonNP->setChecked(false);
    ui->radioButtonUDP->setChecked(false);
    ui->lineEditHost->setEnabled(true);
    mode = CLIENT;
}

void Dialog::setUDPServer()
{
    ui->radioButtonClient->setChecked(false);
    ui->radioButtonNP->setChecked(false);
    ui->lineEditHost->setEnabled(false);
    mode = UDP_SERVER;
}

void Dialog::setNPServer()
{
    ui->radioButtonClient->setChecked(false);
    ui->radioButtonUDP->setChecked(false);
    ui->lineEditHost->setEnabled(false);
    mode = NP_SERVER;
}

Dialog::eMode   Dialog::getMode(){ return mode; }

QString Dialog::getHost() {  return ui->lineEditHost->text(); }
QString Dialog::getPort() {  return ui->lineEditPort->text(); }
int     Dialog::getFontSmall() {  return ui->lineEditFontSmall->text().toInt(); }
int     Dialog::getFontBig()   {  return ui->lineEditFontBig->text().toInt(); }
int     Dialog::getFontTimecode() { return ui->lineEditFontTimecode->text().toInt(); }
