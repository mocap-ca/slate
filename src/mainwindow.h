#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>

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

    QString host;
    int     port;
    bool    sendDataFlag;


private slots:
    void sessionChange(QString);
    void shotChange(QString);
    void dateChange(QString);
    void readData();
};

#endif // MAINWINDOW_H
