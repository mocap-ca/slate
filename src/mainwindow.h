#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QKeyEvent>

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

    QUdpSocket * socket;

    void sendData(QString);

    int     port;
    QString host;
    int     fontSizeSmall;
    int     fontSizeBig;
    bool    sendDataFlag;
    bool    isBound;
    bool    isFullscreen;
    int     saveWidth;
    int     saveHeight;



private :
    void updateFonts();

protected :
    void keyPressEvent( QKeyEvent *);

private slots:
    void sessionChange(QString);
    void shotChange(QString);
    void dateChange(QString);
    void readData();
    void settings();
    void updateScreenSettings();

signals :
    void screenChange();

};

#endif // MAINWINDOW_H
