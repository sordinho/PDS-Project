#ifndef THREAD_H
#define THREAD_H
#include <QThread>
#include <QTcpSocket>
#include "../utils/Constants.h"

class Thread : public QThread{
Q_OBJECT
private:
    //std::list<std::shared_ptr<QTcpSocket>> sockets;
    std::map<qintptr, std::shared_ptr<QTcpSocket>> sockets;
    qintptr socketDescriptor;
public:
    explicit Thread(QObject *parent = nullptr);
    void run();
    void addSocket(qintptr socketDescriptor);
private:
    bool readInsert(QTcpSocket *soc);
    bool readDelete(QTcpSocket *soc);
    bool writeOkMessage(QTcpSocket *soc);
    bool writeErrMessage(QTcpSocket *soc);

signals:
    void error(QTcpSocket::SocketError socketerror);

public slots:
    void readyRead(QTcpSocket *socket);
    void disconnected(QTcpSocket *socket);
};

#endif // THREAD_H
