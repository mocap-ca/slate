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
    port(5678),
    host("127.0.0.1"),
    fontSize(12),
    sendDataFlag(false),
    isBound(false)
{
    ui->setupUi(this);
    connect(ui->lineEditShoot, SIGNAL(textChanged(QString)), this, SLOT(sessionChange(QString)));
    connect(ui->lineEditShot,  SIGNAL(textChanged(QString)), this, SLOT(shotChange(QString)));
    connect(ui->lineEditDate,  SIGNAL(textChanged(QString)), this, SLOT(dateChange(QString)));
    connect(ui->actionSet,     SIGNAL(triggered()), this, SLOT(settings()));
    settings();

}

void MainWindow::settings()
{

    Dialog d(host, port, fontSize, this);
    d.setModal(true);
    if(d.exec() != QDialog::Accepted) return;

    if(isBound)
    {
        socket->close();
        delete socket;
        isBound = false;
    }

    bool server = d.isServer();
    host = d.getHost();
    port = d.getPort().toInt();
    fontSize = d.getFont().toInt();

    QFont font = ui->lineEditShoot->font();

    if(fontSize> 2)
    {
    font.setPointSize( fontSize );
    ui->lineEditShoot->setFont(font);
    ui->lineEditShot->setFont(font);
    ui->lineEditDate->setFont(font);
    }

    socket = new QUdpSocket(this);

    if(server)
    {
        sendDataFlag = false;
        this->statusBar()->showMessage( QString("Starting server on port: %1").arg(port) );
        if( socket->bind(QHostAddress::Any, port) )
        {
            connect( socket, SIGNAL(readyRead()), this, SLOT(readData()));
            isBound=true;
        }
        else
        {
            this->statusBar()->shwMessage( QString("Could not start server") );
            QMessageBox::warning(this, "Socket Error", "Could not start server");
        }
    }
    else
    {
        sendDataFlag=true;
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

        QString msg;
        msg += sender.toString();
        msg += ":";
        msg += QString(data);
        this->statusBar()->showMessage( msg );

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
    qint64 sz= socket->writeDatagram(msg.toUtf8(), QHostAddress(host), port);
    if(sz > 0)
    {
        this->statusBar()->showMessage( QString("Wrote: %1").arg(sz));
    }
    else
    {
        this->statusBar()->showMessage( socket->errorString());
    }
}

void MainWindow::sessionChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("SESSION:");
    data.append(ui->lineEditShoot->text());
    sendData(data);
}

void MainWindow::shotChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("SHOT:");
    data.append(ui->lineEditShot->text());
    sendData(data);
}

void MainWindow::dateChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("DATE:");
    data.append(ui->lineEditDate->text());
    sendData(data);
}


