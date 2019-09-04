#include <QDataStream>
#include <QPixmap>
#include <shared_mutex>
#include "Server.h"

Server::Server(QObject *parent):QTcpServer(parent){}

bool Server::startServer(quint16 port){
    connect(this,SIGNAL(newConnection()),this,SLOT(connection()));

    if(!this->listen(QHostAddress::Any, port)){
        qDebug() << "Server.cpp - startServer()     Could not start server";
        qDebug() << ""; // newLine
        return false;
    }else{
        qDebug() << "Server.cpp - startServer()     Listening to port " << port << "...";
        qDebug() << ""; // newLine
        return true;
    }
}

void Server::connection(){
    QTcpSocket *soc = this->nextPendingConnection();

    qintptr socketDescriptor = soc->socketDescriptor();
    socketsState[soc->socketDescriptor()] = UNLOGGED;
    QMetaObject::Connection *c = new QMetaObject::Connection();
    QMetaObject::Connection *d = new QMetaObject::Connection();

    *c = connect(soc, &QTcpSocket::readyRead, this, [this, c, d, soc, socketDescriptor]{
        QByteArray data;
        if (!readChunck(soc, data, 5)){
            writeErrMessage(soc);
            soc->flush();
            return;
        }

        qDebug() << "Server.cpp - connection()     msg received:" << data;

        if (data.toStdString() == LOGIN_MESSAGE){
            if (logIn(soc)){
                sendFileNames(soc);
                socketsState[socketDescriptor] = LOGGED;
                qDebug() << "                              socketsSize: " << socketsState.size();
                qDebug() << ""; // newLine
            }else{
                //error in login phase
                qDebug() << "                              error login";
                qDebug() << ""; // newLine
                writeErrMessage(soc);
                return;
            }
        }else if (data.toStdString() == REGISTRATION_MESSAGE && socketsState[socketDescriptor] == UNLOGGED){
            if (registration(soc)){
                socketsState[soc->socketDescriptor()] = LOGGED;
            }else{
                writeErrMessage(soc);
                return;
            }
        }else if (data.toStdString() == REQUEST_FILE_MESSAGE && socketsState[socketDescriptor] == LOGGED) {
            /* disconnect from main thread */
            disconnect(*c);
            disconnect(*d);
            delete c;
            delete d;
            socketsState.erase(socketDescriptor);
            qDebug() << "                              socketsSize: " << socketsState.size();
            qDebug() << ""; // newLine
            
            if (!readFileName(soc->socketDescriptor(), soc)){
                writeErrMessage(soc);
            }
        }else{
            qDebug() << "                              error message";
            qDebug() << ""; // newLine
            writeErrMessage(soc);
        }
    }, Qt::DirectConnection);

    *d = connect(soc, &QTcpSocket::disconnected, this, [this, c, d, soc, socketDescriptor]{
        qDebug() << "                              " << socketDescriptor << " Disconnected (form main thread)";
        qDebug() << ""; // newLine
        soc->deleteLater();
        socketsState.erase(socketDescriptor);
    });
}

bool Server::logIn(QTcpSocket *soc){
    /* read user and password on socket*/
    qDebug() << "Server.cpp - logIn()     ---------- LOGIN ----------";

    /* usernameSize */
    readSpace(soc);
    int usernameSize = readNumberFromSocket(soc);
    qDebug() << "                         usernameSize: " << usernameSize;

    readSpace(soc);
    /* username */
    QByteArray username;
    if (!readChunck(soc, username, usernameSize)){
        return false;
    }
    readSpace(soc);

    /* passwordSize */
    int passwordSize = readNumberFromSocket(soc);
    readSpace(soc);
    qDebug() << "                         passwordSize: " <<passwordSize;

    QByteArray password;
    if (!readChunck(soc, password, passwordSize)){
        return false;
    }

    qDebug() << "                         username: " << username << " password: " << password;
    qDebug() << ""; // newLine

    // TODO: richiamo funzione per il login sul db
    usernames[soc->socketDescriptor()] = username;

    return true;
}

bool Server::sendFileNames(QTcpSocket *soc){
    qDebug() << "Server.cpp - sendFileNames()     ---------- LIST OF FILE ----------";
    // TODO gestione file dell'utente

    int nFiles = 2;
    QString fileName[2];
    fileName[0] = "file1";                                     /* file fantoccio: da rimuovere in seguito */
    fileName[1] = "file2";
    QByteArray message(LIST_OF_FILE);

    QByteArray numFiles = convertionNumber(nFiles);
    message.append(" " + numFiles);

    for (int i = 0; i < nFiles; i++ ){
        QByteArray fileNameSize = convertionNumber(fileName[i].size());
        message.append(" " + fileNameSize + " " + fileName[i].toUtf8());
    }

    qDebug() << "                                " << message;
    qDebug() << ""; // newLine

    writeMessage(soc,message);
    return true;
}

bool Server::readFileName(qintptr socketDescriptor, QTcpSocket *soc){
    std::lock_guard<std::mutex> lg(mutexThread);
    qDebug() << "Server.cpp - readFileName()     ---------- REQUEST FOR FILE ----------";
    readSpace(soc);
    int fileNameSize = readNumberFromSocket(soc);
    readSpace(soc);

    QByteArray fileName;
    if (!readChunck(soc, fileName, fileNameSize)){
        writeErrMessage(soc);
        return false;
    }

    qDebug() << "                               " << fileName;

    QString key = fileName;                           /* file name */
    auto result = threads.find(key);

    if (result != threads.end()){
        /* file already open */
        qDebug() << "                               thread for file name aready exist " << fileName;
        qDebug() << ""; // newLine
        threads[key]->addSocket(soc, usernames[socketDescriptor]);                       /* socket transition to secondary thread */
    }else{
        /* file not yet open */
        qDebug() << "                               New thread for file name: " << fileName;
        qDebug() << ""; // newLine
        CRDT *crdt = new CRDT();
        Thread *thread = new Thread(this, crdt, fileName, this);                        /* create new thread */
        threads[key] = std::shared_ptr<Thread>(thread);
        thread->addSocket(soc, usernames[socketDescriptor]);                            /* socket transition to secondary thread */
        thread->start();
    }

    //writeOkMessage(soc);
    return true;
}

bool Server::registration(QTcpSocket *soc){
    qDebug() << "Server.cpp - registration()     ---------- REGISTRATION ----------";
    if (soc == nullptr){
        return false;
    }
    readSpace(soc);
    int sizeUsername = readNumberFromSocket(soc);
    readSpace(soc);

    //username
    QByteArray username;
    if (!readChunck(soc, username, sizeUsername)){
        writeErrMessage(soc);
    }
    readSpace(soc);

    int sizePassword = readNumberFromSocket(soc);
    readSpace(soc);

    //password
    QByteArray password;
    if (!readChunck(soc, password, sizePassword)){
        writeErrMessage(soc);
    }
    readSpace(soc);

    QDataStream in(soc);
    qsizetype sizeAvatar;
    in >> sizeAvatar;
    readSpace(soc);

    qDebug() << "                                username: " << username << " size: " << sizeUsername;
    qDebug() << "                                password: " << password << " size: " << sizePassword;
    qDebug() << "                                avatar size: " << sizeAvatar;

    //avatar
    QByteArray avatarDef;

    if (readChunck(soc, avatarDef, sizeAvatar)){
        writeOkMessage(soc);
    }else{
        writeErrMessage(soc);
    }

    qDebug() << "                                avatar size: " << sizeAvatar << " size read" << avatarDef.size();

    qDebug() << ""; // newLine

    usernames[soc->socketDescriptor()] = username;

    return true;
}

std::shared_ptr<Thread> Server::getThread(QString fileName){
    std::lock_guard<std::mutex> sl(mutexThread);
    auto result = threads.find(fileName);

    if (result != threads.end()){
        return result.operator->()->second;
    }else{
        return std::shared_ptr<Thread>();
    }
}

std::shared_ptr<Thread> Server::addThread(QString fileName){
    std::lock_guard<std::mutex> lg(mutexThread);
    CRDT *crdt = new CRDT();
    std::shared_ptr<Thread> thread = std::make_shared<Thread>(this, crdt, fileName, this);                        /* create new thread */
    threads[fileName] = thread;
    return thread;
}