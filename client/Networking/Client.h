#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <queue>
#include "../utils/Constants.h"
#include "../utils/Identifier.h"
#include "../utils/Character.h"
#include "../utils/Pos.h"
#include "message/Message.h"
#include "../CRDT.h"

class Client: public QObject{
    Q_OBJECT
private:
    QString siteId;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    CRDT *crdt;
    bool reciveOkMessage;
    bool clientIsLogged;
    QMetaObject::Connection c;
    QMetaObject::Connection d;

private:
    std::queue<QByteArray> messages;
    std::queue<Message> incomingInsertMessagesQueue;
    std::queue<Message> incomingDeleteMessagesQueue;

public:
    Client(QObject *parent = nullptr);
    void setCRDT(CRDT *crdt);
    bool writeOnSocket(QString);
    bool insert(QString, Pos pos);
    bool deleteChar(QString str, std::vector<Identifier> pos);
    void resetModel(QString siteId);
    bool connectTo(QString host);
    bool logIn(QString username, QString passsword);
    bool readFileNames();
    bool registration(QString username, QString passsword, QString pathAvatar);
    bool requestForFile(QString fileName);
    bool readInsert();
    bool readDelete();
    bool readReset();
    Message getMessage();
    bool readUsernames();
    bool readRemoveUser();

public slots:
    void onReadyRead();
    void onDisconnect();
    void logOut();

signals:
    void errorConnection();
    void loginFailed();
    void newMessage(Message message);
    void userVerified();
    void fileNames(QStringList fileList);
    void logout();
    void reset(QString siteId);
    void setUsers(QStringList);
    void removeUser(QString);
};

#endif // CLIENT_H
