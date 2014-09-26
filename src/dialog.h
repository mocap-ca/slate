#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

namespace Ui {
class Dialog;
}



class Dialog : public QDialog
{
    Q_OBJECT

public:
    typedef enum mode {
        CLIENT,
        NP_SERVER,
        UDP_SERVER
    } eMode;

    explicit Dialog(QString host, int port, int fontSizeSmall, int fontSizeBig, int fontSizeTimeCode, eMode mode, QWidget *parent = 0);
    ~Dialog();

    eMode getMode();

    QString getHost();
    QString getPort();
    int     getFontSmall();
    int     getFontBig();
    int     getFontTimecode();

private:
    Ui::Dialog *ui;
    eMode mode;

private slots:
    void setClient();
    void setUDPServer();
    void setNPServer();
};

#endif // DIALOG_H
