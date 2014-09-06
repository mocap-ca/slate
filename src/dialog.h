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
    explicit Dialog(QString host, int port, int fontSizeSmall, int fontSizeBig, bool isServer, QWidget *parent = 0);
    ~Dialog();

    bool isServer();
    QString getHost();
    QString getPort();
    int     getFontSmall();
    int     getFontBig();

private:
    Ui::Dialog *ui;

private slots:
    void setClient();
    void setServer();
};

#endif // DIALOG_H
