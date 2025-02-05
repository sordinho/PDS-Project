#include "login.h"
#include "ui_login.h"
#include <QMessageBox>
#include <iostream>

Login::Login(QWidget *parent, Controller *controller): QMainWindow(parent), controller(controller),ui(new Ui::Login) {
    ui->setupUi(this);
    ui->label_3->setVisible(false);
    //connect(this, SIGNAL(showRegistration()), this->parent(), SLOT(showRegistration()));
    //connect(this,SIGNAL(loginSuccessful()),this->parent(), SLOT(showFileFinder()));
}

Login::~Login(){
    delete ui;
}

void Login::setClient(Messanger *messanger) {  //TODO: da rimuovere...
    this->messanger = messanger;
}

void Login::keyPressEvent(QKeyEvent *event) {
    if (event->key() == 16777220)           // Kenter Key is pressed
        on_pushButton_clicked();
}

void Login::on_pushButton_clicked() {
    QString username = ui->username->text();
    QString password = ui->password->text();
    QRegExp rx("^[a-zA-Z0-9]*$");
    if (ui->username->text() == "" && ui->password->text() == "") {
        ui->label_3->setText("Insert username and password");
        ui->label_3->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
        ui->label_3->setVisible(true);
    } else if (ui->username->text() == "") {
        ui->label_3->setText("Insert username");
        ui->label_3->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
        ui->label_3->setVisible(true);
    } else if (ui->password->text() == "") {
        ui->label_3->setText("Insert password");
        ui->label_3->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
        ui->label_3->setVisible(true);
    } else if (rx.indexIn(ui->username->text()) == -1) {
        ui->label_3->setText("Insert valid username");
        ui->label_3->setStyleSheet(QStringLiteral("QLabel{color: red;}"));
        ui->label_3->setVisible(true);
    } else {

        bool result = messanger->logIn(username,password);

        ui->label_3->setVisible(false);
        if (result) {
            controller->startLoadingPopup();
            emit loginSuccessful();
        }
    }
}

void Login::errorConnection(){
    QMessageBox::information(this, "Connection", "Try again, connection not established!");
}

void Login::loginFailed(){
    controller->stopLoadingPopup();
    QMessageBox::warning(this,"Login", "Username and/or password is not correct, try again!");
}

void Login::closeEvent(QCloseEvent *event){
    emit disconnect();
}

void Login::on_pushButton_2_clicked() {
    emit showRegistration();
}

QString Login::getUsername(){
    return ui->username->text();
}

void Login::reset(){
    ui->username->clear();
    ui->password->clear();
}

void Login::resizeEvent(QResizeEvent *event) {
    int centralWidgetX = 0;
    int centralWidgetY = 0;
    int widgetX = 0;
    int widgetY = 0;


    if (width() > ui->centralWidget_2->width()){
        centralWidgetX = width()/2 - ui->centralWidget_2->width()/2;
        widgetX = ui->centralWidget_2->width()/2 - ui->widget->width()/2;
    }else{
        centralWidgetX = 0;
        widgetX = width()/2 - ui->widget->width()/2;
    }

    if (height() > ui->widget->height()){
        widgetY = height()/2 - ui->widget->height()/2;
    } else{
        widgetY = ui->widget->geometry().y();
    }


    ui->centralWidget_2->setGeometry(centralWidgetX, 0, ui->centralWidget_2->width(), height());

    if (widgetX != 0 || widgetY != 0)
        ui->widget->setGeometry(widgetX, widgetY, ui->widget->width(), ui->widget->height());

}