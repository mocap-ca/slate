#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialog.h"
#include <QMessageBox>

MainWindow::~MainWindow()
{
    delete ui;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    socket(NULL),
    port(-1)
{
    ui->setupUi(this);
    connect(ui->lineEditShoot, SIGNAL(textChanged(QString)), this, SLOT(sessionChange(QString)));
    connect(ui->lineEditShot,  SIGNAL(textChanged(QString)), this, SLOT(shotChage(QString)));
    connect(ui->lineEditDate,  SIGNAL(textChanged(QString)), this, SLOT(dateChange(QString)));

    Dialog d(this);
    d.setModal(true);
    d.exec();

    bool server = d.isServer();
    host = d.getHost();
    port = d.getPort().toInt();
    QString font = d.getFont();

    socket = new QUdpSocket(this);

    if(server)
    {
        if( socket->bind(QHostAddress::LocalHost, port) )
        {
            connect( socket, SIGNAL(readyRead()), this, SLOT(readData()));
        }
        else
        {
            QMessageBox::warning(this, "Socket Error", "Could not start server");
        }
    }
    else
    {
        port = -1;
    }
}


void MainWindow::readData()
{
    if (socket == NULL) return;

    QByteArray data;
    QHostAddress sender;
    quint16 senderPort;

    while(socket->hasPendingDatagrams())
    {
        data.resize(socket->pendingDatagramSize());
        socket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        if(data.startsWith("SESSION:"))
            ui->lineEditShoot->setText( data.mid( 8 ));

        if(data.startsWith("SHOT:"))
            ui->lineEditShot->setText( data.mid( 5 ));

        if(data.startsWith("DATE:"))
            ui->lineEditDate->setText( data.mid( 5 ));
    }

}

void MainWindow::sendData(QString msg)
{
    socket->writeDatagram(msg.toUtf8(), QHostAddress(host), port);
}

void MainWindow::sessionChange(QString)
{
    if (port <= 0 ) return;
    QString data("SESSION:");
    data.append(ui->lineEditShoot->text());
    socket->write( data.toUtf8());
}

void MainWindow::shotChange(QString)
{
    if (port <= 0 ) return;
    QString data("SHOT:");
    data.append(ui->lineEditShot->text());
    socket->write( data.toUtf8() );
}

void MainWindow::dateChange(QString)
{
    if (port <= 0 ) return;
    QString data("DATE:");
    data.append(ui->lineEditDate->text());
    socket->write( data.toUtf8() );
}


