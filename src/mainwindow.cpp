#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QXmlStreamReader>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QList>
#include <QCoreApplication>

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
    fontSizeBig(90),
    fontSizeTimecode(120),
    sendDataFlag(false),
    isBound(false),
    isFullscreen(false),
    saveWidth(400),
    saveHeight(300),
    mode(Dialog::NP_SERVER)
{
    ui->setupUi(this);

    saveWidth = this->width();
    saveHeight = this->height();

    this->setWindowTitle("PeelSlate 0.1");

    connect(ui->lineEditA, SIGNAL(textChanged(QString)), this, SLOT(AChange(QString)));
    connect(ui->lineEditB, SIGNAL(textChanged(QString)), this, SLOT(BChange(QString)));
    connect(ui->lineEditC, SIGNAL(textChanged(QString)), this, SLOT(CChange(QString)));
    connect(ui->actionSet,     SIGNAL(triggered()), this, SLOT(settings()));
    connect(this, SIGNAL(screenChange()), this, SLOT(updateScreenSettings()));

    bool showSettings;
    QStringList args = QCoreApplication::arguments();
    for( QStringList::iterator i = args.begin(); i != args.end(); i++)
    {
        QString arg = (*i).toLower();
        if(arg == "-f")
        {
            isFullscreen = true;
            showSettings = false;
        }
    }
    if(showSettings)
        settings();
    else
        emit screenChange();
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
        ui->lineEditA->setReadOnly(true);
        ui->lineEditB->setReadOnly(true);
        ui->lineEditC->setReadOnly(true);
        this->statusBar()->setVisible(false);
        qApp->setOverrideCursor( QCursor(Qt::BlankCursor) );
        //this->setCursor(QCursor(Qt::BlankCursor));
    }
    else
    {
        // Change to regular
        isFullscreen = false;
        updateFonts(); // do this before restoring
        this->setStyleSheet("");
        this->showNormal();
        //this->resize( saveWidth, saveHeight );
        ui->lineEditA->setReadOnly(false);
        ui->lineEditB->setReadOnly(false);
        ui->lineEditC->setReadOnly(false);
        qApp->restoreOverrideCursor();
        //this->setCursor(QCursor(Qt::ArrowCursor));
        this->statusBar()->setVisible(true);
        this->resize( saveWidth, saveHeight );
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
    QFont fontA = ui->lineEditA->font();
    QFont fontB = ui->lineEditC->font();
    int sizeA = isFullscreen ? fontSizeBig : fontSizeSmall;
    int sizeB = isFullscreen ? fontSizeTimecode : fontSizeSmall;
    if( sizeA < 4) sizeA = 4;
    if( sizeB < 4) sizeB = 4;
    fontA.setPointSize( sizeA );
    fontB.setPointSize( sizeB );
    ui->lineEditA->setFont(fontA);
    ui->lineEditB->setFont(fontA);
    ui->lineEditC->setFont(fontB);
    this->show();
    this->update();
    ui->verticalLayout->update();
}

void MainWindow::settings()
{

    Dialog d(host, port, fontSizeSmall, fontSizeBig, fontSizeTimecode, mode, this);
    d.setModal(true);
    if(d.exec() != QDialog::Accepted) return;

    if(isBound)
    {
        socket->close();
        delete socket;
        isBound = false;
    }

    mode = d.getMode();
    host = d.getHost();
    port = d.getPort().toInt();
    fontSizeSmall = d.getFontSmall();
    fontSizeBig   = d.getFontBig();

    socket = new QUdpSocket(this);
    NpTakeSocket = new QUdpSocket(this);
    NpTcSocket = new QUdpSocket(this);

    if(mode == Dialog::UDP_SERVER)
    {
        sendDataFlag = false;
        this->statusBar()->showMessage( QString("Starting server on port: %1").arg(port) );
        if( socket->bind(QHostAddress::Any, port) )
        {
            connect( socket, SIGNAL(readyRead()), this, SLOT(readData()));
            isBound = true;
            isFullscreen = true;
        }
        else
        {
            this->statusBar()->showMessage( QString("Could not start server") );
            QMessageBox::warning(this, "Socket Error", "Could not start server");
        }


    }

    if(mode == Dialog::NP_SERVER)
    {
        // This socket reads the current take from port 1512 - UDP
        if(NpTakeSocket->bind(1512))
        {
            connect( NpTakeSocket, SIGNAL(readyRead()), this, SLOT(NpTakeReadData()));
        }

        // Multiscast socket for NatNet realtime feed (for timecode)
        if(NpTcSocket->bind(QHostAddress::Any,1511))
        {
            // We need to find a value for mciface - this is a hack for now...
            QNetworkInterface mciface;
            bool found = false;
            QList<QNetworkInterface> il(QNetworkInterface::allInterfaces());
            for(QList<QNetworkInterface>::iterator i = il.begin(); i != il.end(); i++)
            {
                QList<QNetworkAddressEntry> ade( (*i).addressEntries());
                for(QList<QNetworkAddressEntry>::iterator j=ade.begin(); j!=ade.end(); j++)
                {
                    if( (*j).ip().toString() == "169.254.157.20")
                    {
                        mciface = *i;
                        NpTcSocket->setMulticastInterface(*i);
                        found = true;
                        break;
                    }
                }
            }
            if(!found)
            {
                QMessageBox::warning(this, "error", "Could not find multicast interface");
            }
            else
            {
                if(!NpTcSocket->joinMulticastGroup( QHostAddress("239.255.42.99"), mciface))
                {
                    QMessageBox::warning(this, "error", "Could not join multicast");
                }
                else
                {
                    connect( NpTcSocket, SIGNAL(readyRead()), this, SLOT(NpTcReadData()));
                    isFullscreen = true;
                }
            }
        }
    }
    else
    {
        sendDataFlag=true;
    }

    updateFonts();
    updateScreenSettings();
}

void MainWindow::NpTakeReadData()
{
    QByteArray data;
    QHostAddress sender;
    quint16 senderPort;

    while(NpTakeSocket->hasPendingDatagrams())
    {
        data.resize(NpTakeSocket->pendingDatagramSize());
        NpTakeSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        QXmlStreamReader xml(data);

        if(xml.hasError() || xml.atEnd()) return;

        QXmlStreamReader::TokenType token = xml.readNext();

        if(xml.hasError()) return;

        if(xml.name() == "CaptureStop")
        {
            ui->lineEditB->setText("");
            break;
        }

        while( !xml.atEnd())
        {

            if(xml.hasError()) break;
            token = xml.readNext();

            if(token == QXmlStreamReader::StartElement)
            {
                if(xml.name() == "Name")
                {
                    ui->lineEditB->setText( xml.attributes().value("VALUE").toString() );
                }
            }
        }
    }
}

void MainWindow::NpTcReadData()
{
    QByteArray data;
    QHostAddress sender;
    quint16 senderPort;

    while(NpTcSocket->hasPendingDatagrams())
    {
        data.resize(NpTcSocket->pendingDatagramSize());
        NpTcSocket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        const char *ptr = data.data();

        size_t i = 0;

        int messageId = 0;
        int nBytes = 0;
        memcpy(&messageId, ptr, 2);
        if(messageId != 7) continue;
        memcpy(&nBytes, ptr+2, 2);

        int tc;
        memcpy(&tc, ptr + nBytes - 14, 4);

        int h = (tc>>24) & 255;
        int m = (tc>>16) & 255;
        int s = (tc>>8)  & 255;
        int f =  tc & 255;

        QString val;
        val.sprintf("%02d:%02d:%02d:%02d", h, m, s, f);

        ui->lineEditC->setText( val );

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
            ui->lineEditA->setText( data.mid( 8 ));

        if(data.startsWith("SHOT:"))
            ui->lineEditB->setText( data.mid( 5 ));

        if(data.startsWith("DATE:"))
            ui->lineEditC->setText( data.mid( 5 ));
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

void MainWindow::AChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("SESSION:");
    data.append(ui->lineEditA->text());
    sendData(data);
}

void MainWindow::BChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("SHOT:");
    data.append(ui->lineEditB->text());
    sendData(data);
}

void MainWindow::CChange(QString)
{
    if (!sendDataFlag ) return;
    QString data("DATE:");
    data.append(ui->lineEditC->text());
    sendData(data);
}


