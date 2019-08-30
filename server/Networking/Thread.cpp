#include "Thread.h"
#include <QDataStream>

class Identifier;

class Character;

Thread::Thread(QObject *parent, CRDT *crdt, QString filename) : QThread(parent), crdt(crdt), filename(filename) {
	// Create new timer
	saveTimer = new QTimer(this);

	// Setup signal and slot
	connect(saveTimer, SIGNAL(timeout()), this, SLOT(saveCRDTToFile()));
}

void Thread::run() {
	exec();
}

void Thread::addSocket(QTcpSocket *soc) {
	qintptr socketDescriptor = soc->socketDescriptor();
	/* insert new socket into structure */
	sockets[socketDescriptor] = std::shared_ptr<QTcpSocket>(soc);
	qDebug() << "size" << sockets.size();

	/* connect socket and signal */
	connect(soc, &QAbstractSocket::readyRead, this, [this, soc]() {
		qDebug() << soc;
		Thread::readyRead(soc);
	}, Qt::DirectConnection);

	connect(soc, &QAbstractSocket::disconnected, this, [this, soc, socketDescriptor]() {
		qDebug() << soc;
		Thread::disconnected(soc, socketDescriptor);
	}, Qt::DirectConnection);

	qDebug() << socketDescriptor << " Client connected" << soc;
}

bool Thread::readInsert(QTcpSocket *soc){
	qDebug() << "-------------READ INSERT-------------";
	readSpace(soc);
	int sizeString = readNumberFromSocket(soc);
	readSpace(soc);

	QByteArray letter;
	if (!readChunck(soc, letter, sizeString)){
		return false;
	}
	readSpace(soc);

	//siteID
	int sizeSiteId = readNumberFromSocket(soc);
	readSpace(soc);

	QByteArray siteId;
	if (!readChunck(soc, siteId, sizeSiteId)){
		return false;
	}
	readSpace(soc);

	int posChInt = readNumberFromSocket(soc);
	readSpace(soc);

	int posLineInt = readNumberFromSocket(soc);

	qDebug() << "ch: "<<letter << "siteId: " << siteId << " posCh: " << posChInt << " posLine: " << posLineInt;

    Pos startPos{posChInt, posLineInt};

    for(char c : letter) {
        Character character = crdt->handleInsert(c, startPos, QString{siteId});
        this->insert(QString{character.getValue()}, character.getSiteId(), character.getPosition());

        // increment startPos
        if(c == '\n') {
            startPos.resetCh();
            startPos.incrementLine();
        } else {
            startPos.incrementCh();
        }
    }

	needToSaveFile = true;
	if (!timerStarted) {
		saveTimer->start(saveInterval);
		timerStarted = true;
	}
	return true;
}


void Thread::readyRead(QTcpSocket *soc){
	QByteArray data;
	qDebug() << data;
	if (!readChunck(soc, data, 5)){
		/* eccezione */
		writeErrMessage(soc);
		return;
	}

	if (data.toStdString() == INSERT_MESSAGE){
		if (!readInsert(soc)){
			writeErrMessage(soc);
		}
	}else if (data.toStdString() == DELETE_MESSAGE){
		if (!readDelete(soc)){
			writeErrMessage(soc);
		}
	}else{
		writeErrMessage(soc);
	}
}


void Thread::saveCRDTToFile() {
	if (needToSaveFile)
		crdt->saveCRDT(filename);
}

bool Thread::readDelete(QTcpSocket *soc){
    qDebug() << "-------------READ DELETE-------------";
    readSpace(soc);
    QByteArray letter;
    if (!readChunck(soc, letter, 1)){
        return false;
    }
    readSpace(soc);

    //siteID
    int sizeSiteId = readNumberFromSocket(soc);
    readSpace(soc);

    QByteArray siteId;
    if (!readChunck(soc, siteId, sizeSiteId)){
        return false;
    }
    readSpace(soc);

    int size = readNumberFromSocket(soc);
    qDebug() << " size:" << size << " size Int:" << size;
    readSpace(soc);
    std::vector<Identifier> position;
    qDebug() << letter;

    for (int i = 0; i < size; i++){
        int pos = readNumberFromSocket(soc);
        Identifier identifier(pos, siteId);
        position.push_back(identifier);
        qDebug() << " pos:" << pos;
        if (i != size - 1 || size != 1){
            readSpace(soc);
        }
    }

    Character character(letter[0], 0, siteId, position);

    crdt->handleDelete(character);

    // broadcast
    this->deleteChar(QString{character.getValue()}, character.getSiteId(), character.getPosition());

	needToSaveFile = true;
	if (!timerStarted) {
		saveTimer->start(saveInterval);
		timerStarted = true;
	}
    return true;
}

void Thread::insert(QString str, QString siteId,std::vector<Identifier> pos){
    qDebug() << "-------------WRITE INSERT-------------";
    QByteArray message(INSERT_MESSAGE);
    QByteArray strSize = convertionNumber(str.size());
    QByteArray siteIdSize = convertionNumber(siteId.size());
    QByteArray posSize = convertionNumber(pos.size());

    message.append(" " + strSize + " " + str.toUtf8() + " " + siteIdSize + " " + siteId.toUtf8() + " " + posSize + " ");
    QByteArray position;

    for (int i = 0; i < pos.size(); i++){
        position.append(convertionNumber(pos[i].getDigit()));
        if (i != pos.size() - 1 || pos.size() != 1){
            position.append(" ");
        }
    }
    message.append(position);
    qDebug() << message;

    //broadcast
    for(std::pair<qintptr, std::shared_ptr<QTcpSocket>> socket : sockets){
        writeMessage(socket.second.get(), message);
    }
}

void Thread::deleteChar(QString str,  QString siteId, std::vector<Identifier> pos){
    qDebug() << "-------------WRITE DELETE-------------";
    QByteArray message(DELETE_MESSAGE);
    QByteArray siteIdSize = convertionNumber(siteId.size());
    QByteArray posSize = convertionNumber(pos.size());

    qDebug() << pos.size();
    message.append(" " + str.toUtf8() + " " + siteIdSize + " " + siteId.toUtf8() + " "+ posSize + " ");
    QByteArray position;

    for (int i = 0; i < pos.size(); i++){
        position.append(convertionNumber(pos[i].getDigit()));
        if (i != pos.size() - 1 || pos.size() != 1){
            position.append(" ");
        }
    }
    message.append(position);
    qDebug() << message;

    //broadcast
    for(std::pair<qintptr, std::shared_ptr<QTcpSocket>> socket : sockets){
        writeMessage(socket.second.get(), message);
    }
}

void Thread::disconnected(QTcpSocket *soc, qintptr socketDescriptor){
    qDebug() << socketDescriptor << " Disconnected";
    QTcpSocket socket;
    socket.setSocketDescriptor(socketDescriptor);
    socket.deleteLater();
    sockets.erase(soc->socketDescriptor());

}
