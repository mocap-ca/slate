#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QKeyEvent>
#include "dialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QUdpSocket* socket;
    QUdpSocket* NP1socket;
    QUdpSocket* NP2socket;

    void sendData(QString);

    int     port;
    QString host;
    int     fontSizeSmall;
    int     fontSizeBig;
    int     fontSizeTimecode;
    bool    sendDataFlag;
    bool    isBound;
    bool    isFullscreen;
    int     saveWidth;
    int     saveHeight;
    Dialog::eMode mode;



private :
    void updateFonts();

protected :
    void keyPressEvent( QKeyEvent *);

private slots:
    void AChange(QString);
    void BChange(QString);
    void CChange(QString);
    void readData();
    void NP1readData();
    void NP2readData();
    void settings();
    void updateScreenSettings();

signals :
    void screenChange();

};

#endif // MAINWINDOW_H
