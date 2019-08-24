#ifndef LOGIN_H
#define LOGIN_H

#include <QMainWindow>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include "../Networking/Client.h"
#include "registration.h"

namespace Ui {
class Login;
}

class Login : public QMainWindow
{
    Q_OBJECT
public:
    explicit Login(QWidget *parent = nullptr);
    void setClient(Client *client);
    ~Login();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void errorConnection();
    void loginFailed();
   // void loginDone();
   // void onOkButtonClicked();

protected:
    void closeEvent(QCloseEvent *event);

signals:
    void disconnect();
    void loginSuccessful();

private:
    Ui::Login *ui;
    Client* client;
    Registration* reg;
};

#endif // MAINWINDOW_H
