#include "Thread.h"
#include "../../client/utils/Identifier.h"
#include <QDataStream>

Thread::Thread(QObject *parent):QThread(parent){}

void Thread::run(){
    exec();
}

void Thread::addSocket(qintptr socketDescriptor){
    QTcpSocket *soc = new QTcpSocket();

    // set the ID
    if(!soc->setSocketDescriptor(socketDescriptor)){
        // something's wrong, we just emit a signal
        emit error(soc->error());
        return;
    }

    /* insert new socket into structure */
    sockets[socketDescriptor] = std::shared_ptr<QTcpSocket>(soc);
    qDebug() << "size" << sockets.size();

    /* connect socket and signal */
    connect(soc, &QAbstractSocket::readyRead, this, [this,soc](){
        qDebug() << soc;
        Thread::readyRead(soc);
    }, Qt::DirectConnection);

    connect(soc, &QAbstractSocket::disconnected, this, [this,soc](){
        qDebug() << soc;
        Thread::disconnected(soc);
    });

    qDebug() << socketDescriptor << " Client connected" << soc;
}

void Thread::readyRead(QTcpSocket *soc){
    QByteArray data = soc->read(5);

    if (data.toStdString() == INSERT_MESSAGE){
        readInsert(soc);
        writeOkMessage(soc);
    }else if (data.toStdString() == DELETE_MESSAGE){
        readDelete(soc);
        writeOkMessage(soc);
    }else{
        writeErrMessage(soc);
    }
    soc->waitForBytesWritten(3000);
    std::vector<int> numbers{1,2,3,4,5};
    insert("c", "123", numbers);
}

bool Thread::readInsert(QTcpSocket *soc){
    qDebug() << "-------------INS-------------";
    bool ok;
    QDataStream in(soc);
    int sizeString;
    in >> sizeString;
    soc->read(1);

    QByteArray letter = soc->read(sizeString);
    soc->read(1);

    //siteID
    QByteArray sizeSiteId = soc->read(1);
    soc->read(1);
    QByteArray siteID = soc->read(sizeSiteId.toHex().toInt(&ok,16));
    qDebug()<< siteID << " size" << sizeSiteId.toHex().toInt(&ok,16);
    soc->read(1);

    QByteArray pos = soc->read(1);
    qDebug()<< letter << " pos" << pos.toHex().toInt(&ok,16);
    soc->read(1);
    /*std::vector<int> position;
    qDebug() << letter;
    
    for (int i = 0; i < size.toHex().toInt(&ok,16); i++){
        int pos = soc->read(1).toHex().toInt(&ok,16);
        position.push_back(pos);
        qDebug() << " pos:" << pos;
        if (i != size.toHex().toInt(&ok,16) - 1 || size.toHex().toInt(&ok,16) != 1){
            soc->read(1);
        }
    }*/
    return true;
}

bool Thread::readDelete(QTcpSocket *soc){
    qDebug() << "-------------ERR-------------";
    soc->read(1);
    QByteArray letter = soc->read(1);
    soc->read(1);
    QByteArray size = soc->read(1);
    bool ok;
    qDebug() << " size:" << size << " size Int:" << size.toHex().toInt(&ok,16);
    soc->read(1);
    std::vector<int> position;
    qDebug() << letter;

    for (int i = 0; i < size.toHex().toInt(&ok,16); i++){
        int pos = soc->read(1).toHex().toInt(&ok,16);
        position.push_back(pos);
        qDebug() << " pos:" << pos;
        if (i != size.toHex().toInt(&ok,16) - 1 || size.toHex().toInt(&ok,16) != 1){
            soc->read(1);
        }
    }
    return true;
}

void Thread::insert(QString str, QString siteId,std::vector<int> pos){
    QByteArray message(INSERT_MESSAGE);
    QByteArray data;
    QDataStream in(&data,  QIODevice::WriteOnly);
    data.append(" ");
    in << str.size();

    data.append(" " + str + " " + siteId.size() + " " + siteId + " "+ pos.size() + " ");
    QByteArray position;

    for (int i = 0; i < pos.size(); i++){
        position.append(pos[i]);
        if (i != pos.size() - 1 || pos.size() != 1){
            position.append(" ");
        }
    }
    data.append(position);
    message.append(data);
    qDebug() << message;

    //broadcast
    for(std::pair<qintptr, std::shared_ptr<QTcpSocket>> socket : sockets){
        socket.second->write(message);
    }
}

/*void Thread::insert(QString str, std::vector<Identifier> pos){
    QByteArray message(INSERT_MESSAGE);
    QByteArray data;
    QDataStream in(&data,  QIODevice::WriteOnly);
    data.append(" ");
    in << str.size();

    data.append(" " + str + " " + pos.size() + " ");
    QByteArray position;

    for (int i = 0; i < pos.size(); i++){
        position.append(pos[i].getDigit());
        if (i != pos.size() - 1 || pos.size() != 1){
            position.append(" ");
        }
    }
    data.append(position);
    message.append(data);
    qDebug() << message;

    //broadcast
    for(std::pair<qintptr, std::shared_ptr<QTcpSocket>> socket : sockets){
        socket.second->write(message);
    }
}*/

void Thread::disconnected(QTcpSocket *soc){
    qDebug() << soc->socketDescriptor() << " Disconnected";

    soc->deleteLater();
    sockets.erase(soc->socketDescriptor());
}

bool Thread::writeOkMessage(QTcpSocket *soc){
    if (soc == nullptr){
        return false;
    }

    soc->write(OK_MESSAGE);
    if (soc->waitForBytesWritten(30)){
        qDebug() << "Ok, scritto";
        return true;
    }else{
        qDebug() << "Ok, non scritto";
        return false;
    }
}

bool Thread::writeErrMessage(QTcpSocket *soc){
    if (soc == nullptr){
        return false;
    }

    soc->write(ERR_MESSAGE);
    if (soc->waitForBytesWritten(30)){
        qDebug() << "Err, scritto";
        return true;
    }else{
        qDebug() << "Err, non scritto";
        return false;
    }
}
