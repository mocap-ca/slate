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
    fontSizeSmall(12),
    fontSizeBig(120),
    sendDataFlag(false),
    isBound(false),
    isFullscreen(false),
    saveWidth(-1),
    saveHeight(-1)
{
    ui->setupUi(this);

    saveWidth = this->width();
    saveHeight = this->height();

    this->setWindowTitle("PeelSlate 0.1");


    connect(ui->lineEditShoot, SIGNAL(textChanged(QString)), this, SLOT(sessionChange(QString)));
    connect(ui->lineEditShot,  SIGNAL(textChanged(QString)), this, SLOT(shotChange(QString)));
    connect(ui->lineEditDate,  SIGNAL(textChanged(QString)), this, SLOT(dateChange(QString)));
    connect(ui->actionSet,     SIGNAL(triggered()), this, SLOT(settings()));
    connect(this, SIGNAL(screenChange()), this, SLOT(updateScreenSettings()));
    settings();
}

void MainWindow::updateScreenSettings()
{
    if(isFullscreen)
    {
        // Change to full
        isFullscreen = true;
        this->setStyleSheet("QLineEdit { border:none; background: black; color: white; }  QMainWindow { background: black; }");
        this->showFullScreen();
        saveWidth = this->width();
        saveHeight = this->height();
        updateFonts();
        ui->lineEditDate->setReadOnly(true);
        ui->lineEditShot->setReadOnly(true);
        ui->lineEditShoot->setReadOnly(true);
        this->statusBar()->setVisible(false);
        this->setCursor(QCursor(Qt::BlankCursor));
    }
    else
    {
        // Change to regular
        isFullscreen = false;
        updateFonts(); // do this before restoring
        this->setStyleSheet("");
        this->showNormal();
        this->resize( saveWidth, saveHeight );
        ui->lineEditDate->setReadOnly(false);
        ui->lineEditShot->setReadOnly(false);
        ui->lineEditShoot->setReadOnly(false);
        this->show();
        this->setCursor(QCursor(Qt::ArrowCursor));
        this->statusBar()->setVisible(true);
    }

    ui->menuBar->setVisible(!isFullscreen);
}

void MainWindow::keyPressEvent(QKeyEvent *k)
{
    if(k->key() == Qt::Key_Escape && isFullscreen)
    {
        isFullscreen = false;
        emit screenChange();
    }
    if(k->modifiers() & Qt::ControlModifier && k->modifiers() & Qt::AltModifier) return;

    if( k->key() == Qt::Key_Q )
        QApplication::quit();

    if( k->key() == Qt::Key_O )
        settings();

    if( k->key() == Qt::Key_Enter || k->key() == Qt::Key_Space)
    {
        isFullscreen = !this->isFullScreen();
        emit screenChange();


    }
}

void MainWindow::updateFonts()
{
    QFont font = ui->lineEditShoot->font();
    int size = isFullscreen ? fontSizeBig : fontSizeSmall;
    if( size < 4) size =4;
    font.setPointSize( size );
    ui->lineEditShoot->setFont(font);
    ui->lineEditShot->setFont(font);
    ui->lineEditDate->setFont(font);
    this->show();
    this->update();
    ui->verticalLayout->update();
}

void MainWindow::settings()
{

    Dialog d(host, port, fontSizeSmall, fontSizeBig, isBound, this);
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
    fontSizeSmall = d.getFontSmall();
    fontSizeBig   = d.getFontBig();

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
            this->statusBar()->showMessage( QString("Could not start server") );
            QMessageBox::warning(this, "Socket Error", "Could not start server");
        }
    }
    else
    {
        sendDataFlag=true;
    }


    updateFonts();
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


