//
// Created by Eugenio Marinelli on 2019-08-21.
//

#include <QMessageBox>
#include "MainWindow.h"

MainWindow::MainWindow(QString siteId)
        : editor(new Editor(siteId,this)),
          login(new Login(this)),
          crdt(new CRDT()),
          client(new Client(this)),
          controller(new Controller(this->crdt,this->editor,this->client)),
          connection(new Connection(this)),
          registration(new Registration(this)),
          finder(new ShowFiles(this))
{
    this->login->setClient(this->client);
    this->client->setCRDT(this->crdt);


    /* define connection */
    connect(this->connection, SIGNAL(connectToAddress(QString)),this, SLOT(connectClient(QString)));
    connect(this->client, SIGNAL(fileNames(QStringList)),this, SLOT(showFileFinder(QStringList)));
    connect(this->client, &Client::errorConnection, this, &MainWindow::errorConnection);
    connect(this->client, &Client::logout, this, &MainWindow::showLogin);
    connect(this->finder, &ShowFiles::logout, this->client, &Client::logOut);
    connect(this->editor, &Editor::showFinder, this, &MainWindow::showFileFinderOtherView);
}

void MainWindow::show() {
    this->connection->show();
}

void MainWindow::showFileFinder(QStringList fileList){
    if (user == nullptr){
        this->login->close();
        this->editor->close();
        this->finder->addFiles(fileList);
        this->finder->show();
        user = new User(login->getUsername());
        user->setFileLis(fileList);
        user->setIsLogged(true);
    }else{
        this->login->close();
        this->editor->close();
        delete finder;
        finder = new ShowFiles(this);
        this->finder->addFiles(fileList);
        connect(this->finder, &ShowFiles::logout, this->client, &Client::logOut);
        connect(this->editor, &Editor::showFinder, this, &MainWindow::showFileFinderOtherView);
        this->finder->show();
        //user = new User(login->getUsername());
        user->setFileLis(fileList);
    }
}

void MainWindow::showFileFinderOtherView(){
    this->login->close();
    this->editor->close();
    delete finder;
    finder = new ShowFiles(this);
    this->finder->addFiles(user->getFileList());
    this->editor->close();
    connect(this->finder, &ShowFiles::logout, this->client, &Client::logOut);
    connect(this->editor, &Editor::showFinder, this, &MainWindow::showFileFinderOtherView);
    this->finder->show();
}

void MainWindow::showEditor(){
    this->login->close();
    this->editor->show();
}

void MainWindow::showLogin(){
    this->finder->close();
    this->editor->close();
    this->registration->close();
    this->login->show();
}

void MainWindow::connectClient(QString address) {
    bool res=this->client->connectTo(address);
    if(res) {
        this->connection->close();
        this->login->show();
    }
}

void MainWindow::showRegistration(){
    this->login->close();
    this->registration->show();
}

void MainWindow::requestForFile(QString filename){
    bool result = this->client->requestForFile(filename);

    if (result){
        this->finder->close();
        this->editor->show();
    }
}

void MainWindow::errorConnection(){
    QMessageBox::information(this, "Connection", "Try again, connection not established!");
}