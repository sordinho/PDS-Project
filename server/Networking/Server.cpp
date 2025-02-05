#include <QDataStream>

#include <shared_mutex>
#include <utility>
#include "Server.h"
#include "../SimpleCrypt/SimpleCrypt.h"
#include "../Utils/Utilities.h"

Server::Server(QObject *parent) : QTcpServer(parent) {}

/**
 * This method starts server's listening on specific port
 * @param port
 * @return
 */
bool Server::startServer(quint16 port) {
	connect(this, SIGNAL(newConnection()), this, SLOT(connection()), Qt::QueuedConnection);
	if (!this->listen(QHostAddress::Any, port)) {
		//qDebug() << "Server.cpp - startServer()     Could not start server";
		//qDebug() << ""; // newLine
		return false;
	} else {
		//qDebug() << "Server.cpp - startServer()     Listening to port " << port << "...";
		//qDebug() << ""; // newLine
		return checkAndCreateSaveDir();
	}
}

/**
 * This method starts a new handling connection
 */
void Server::connection() {
	std::unique_lock<std::shared_mutex> socketsMutex(mutexSockets);
	QTcpSocket *soc = this->nextPendingConnection();

	try {
		qintptr socketDescriptor = soc->socketDescriptor();
		socketsState[socketDescriptor] = UNLOGGED;
		sockets[socketDescriptor] = soc;
		auto *connectReadyRead = new QMetaObject::Connection();
		auto *connectDisconnected = new QMetaObject::Connection();
		soc->setParent(nullptr);

		connectionSlot(soc, connectReadyRead, connectDisconnected);
	} catch (...) {
		soc->deleteLater();
	}
}

void Server::connectionSlot(QTcpSocket *soc, QMetaObject::Connection *connectReadyRead,
							QMetaObject::Connection *connectDisconnected) {
	qintptr socketDescriptor = soc->socketDescriptor();

	*connectReadyRead = connect(soc, &QTcpSocket::readyRead, this,
								[this, connectReadyRead, connectDisconnected, soc, socketDescriptor] {
									readyRead(connectReadyRead, connectDisconnected, soc, socketDescriptor);
								}, Qt::QueuedConnection);

	*connectDisconnected = connect(soc, &QTcpSocket::disconnected, this,
								   [this, connectReadyRead, connectDisconnected, soc, socketDescriptor] {
									   disconnected(connectReadyRead, connectDisconnected, soc, socketDescriptor);
								   }, Qt::QueuedConnection);
}

/**
 * This method allows to start reading from socket and calls different methods
 * that handles different type of message.
 */
void Server::readyRead(QMetaObject::Connection *connectReadyRead, QMetaObject::Connection *connectDisconnected,
					   QTcpSocket *soc, qintptr socketDescriptor) {
	QByteArray data;
    if (soc == nullptr)
        return;

	if (soc->bytesAvailable() == 0) {
        return;
    }

	if (!readChunck(soc, data, 5)) {
		writeErrMessage(soc);
		return;
	}

	//qDebug() << "Server.cpp - connection()     msg received:" << data;
	try {
		if (data.toStdString() == LOGIN_MESSAGE && socketsState[socketDescriptor] == UNLOGGED) {
			std::unique_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
			std::unique_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			if (logIn(soc)) {
				if (!sendUser(soc)) {
					writeErrMessage(soc, LOGIN_MESSAGE);
					return;
				}
				if (!sendFileNames(soc)) {
					writeErrMessage(soc, LOGIN_MESSAGE);
					return;
				}
				socketsState[socketDescriptor] = LOGGED;
				//qDebug() << "                              socketsSize: " << socketsState.size();
				//qDebug() << ""; // newLine
			} else {
				//error in login phase
				//qDebug() << "                              error login";
				//qDebug() << ""; // newLine
				writeErrMessage(soc, LOGIN_MESSAGE);
				return;
			}
		} else if (data.toStdString() == REGISTRATION_MESSAGE && socketsState[socketDescriptor] == UNLOGGED) {
			std::unique_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
			std::unique_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			if (registration(soc)) {
				if (!sendUser(soc)) {
					writeErrMessage(soc, LOGIN_MESSAGE);
					return;
				}
				if (!sendFileNames(soc)) {
					writeErrMessage(soc, LOGIN_MESSAGE);
					return;
				}
				socketsState[socketDescriptor] = LOGGED;
			} else {
				writeErrMessage(soc, REGISTRATION_MESSAGE);
				return;
			}
		} else if (data.toStdString() == REQUEST_FILE_MESSAGE && socketsState[socketDescriptor] == LOGGED) {
			std::unique_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			std::unique_lock<std::shared_mutex> threadsMutex(mutexThread);
			/* disconnect from main thread */
			disconnect(*connectReadyRead);
			if (readFileName(socketDescriptor, soc)) {
				disconnect(*connectDisconnected);
				delete connectReadyRead;
				delete connectDisconnected;
				socketsState.erase(socketDescriptor);
				//qDebug() << "                              socketsSize: " << socketsState.size();
				//qDebug() << ""; // newLine
			} else {
				/* connect from main thread */
				if (writeErrMessage(soc, REQUEST_FILE_MESSAGE))
					connectionSlot(soc, connectReadyRead, connectDisconnected);
			}
		} else if (data.toStdString() == EDIT_ACCOUNT && socketsState[socketDescriptor] == LOGGED) {
			std::unique_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
			std::unique_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			std::unique_lock<std::shared_mutex> threadsMutex(mutexThread);
			std::unique_lock<std::shared_mutex> socketsMutex(mutexSockets);
			if (readEditAccount(soc)) {
				sendUser(soc);
				sendFileNames(soc);
			} else {
				writeErrMessage(soc, EDIT_ACCOUNT);
			}
		} else if (data.toStdString() == SHARE_CODE && socketsState[socketDescriptor] == LOGGED) {
			std::shared_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			if (!readShareCode(soc)) {
				writeErrMessage(soc, SHARE_CODE);
			}
		} else if (data.toStdString() == REQUEST_USERNAME_LIST_MESSAGE && socketsState[socketDescriptor] == LOGGED) {
			if (!readRequestUsernameList(soc)) {
				writeErrMessage(soc, REQUEST_USERNAME_LIST_MESSAGE);
			}
		} else if (data.toStdString() == FILE_INFORMATION_CHANGES && socketsState[socketDescriptor] == LOGGED) {
			std::shared_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
			std::shared_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			std::unique_lock<std::shared_mutex> threadsMutex(mutexThread);
			std::unique_lock<std::shared_mutex> socketsMutex(mutexSockets);
			if (!readFileInformationChanges(soc)) {
				writeErrMessage(soc, FILE_INFORMATION_CHANGES);
			}
		} else if (data.toStdString() == DELETE_FILE && socketsState[socketDescriptor] == LOGGED) {
			std::shared_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
			std::shared_lock<std::shared_mutex> usernamesMutex(mutexUsernames);
			std::unique_lock<std::shared_mutex> threadsMutex(mutexThread);
			std::unique_lock<std::shared_mutex> socketsMutex(mutexSockets);
			if (!readDeleteFile(soc)) {
				writeErrMessage(soc, DELETE_FILE);
			}
		} else {
			//qDebug() << "                              error message";
			//qDebug() << ""; // newLine
			writeErrMessage(soc);
		}
	} catch (...) {
		writeErrMessage(soc);
		disconnected(connectReadyRead, connectDisconnected, soc, socketDescriptor);
	}
}

/**
 *
 * This method reads account and password from socket
 * @param soc
 * @return result of reading from socket
 */
bool Server::logIn(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - logIn()     ---------- LOGIN ----------";
	if (soc == nullptr)
		return false;

	/* read user and password on socket*/
	/* usernameSize */
	if (!readSpace(soc)) {
		return false;
	}
	int usernameSize = readNumberFromSocket(soc);
	//qDebug() << "                         usernameSize: " << usernameSize;

	if (!readSpace(soc)) {
		return false;
	}

	/* username */
	QString username;
	if (!readQString(soc, username, usernameSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* passwordSize */
	int passwordSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}
	//qDebug() << "                         passwordSize: " << passwordSize;

	QString password;
	if (!readQString(soc, password, passwordSize)) {
		return false;
	}

	//qDebug() << "                         username: " << username << " password: " << password;
	//qDebug() << ""; // newLine

	/* Check if user is already logged in */
	for (const std::pair<quintptr, QString> pair : allUsernames) {
		if (pair.second == QString(username))
			return false;
	}

	/* DB user authentication */
	bool authentication = DB.authenticateUser(QString(username), QString(password));
	if (authentication) {
		usernames[soc->socketDescriptor()] = username;
		allUsernames[soc->socketDescriptor()] = username;
		return true;
	} else
		return false;
}

/**
 * This method reads username and password from the client for registration
 * @param soc
 * @return result of reading from socket
 */
bool Server::registration(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - registration()     ---------- REGISTRATION ----------";
	if (soc == nullptr) {
		return false;
	}

	/* read user, password and avatar on socket*/
	/* usernameSize */
	if (!readSpace(soc)) {
		return false;
	}
	int usernameSize = readNumberFromSocket(soc);
	readSpace(soc);

	/* username */
	QString username;
	if (!readQString(soc, username, usernameSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* passwordSize */
	int passwordSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* password */
	QString password;
	if (!readQString(soc, password, passwordSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* avatarSize */
	int avatarSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	//qDebug() << "                                username: " << username << " size: " << usernameSize;
	//qDebug() << "                                password: " << password << " size: " << passwordSize;
	//qDebug() << "                                avatar size: " << avatarSize;

	/* avatar */
	QByteArray avatarDef;

	if (!readChunck(soc, avatarDef, avatarSize)) {
		return false;
	}

	//qDebug() << "                                avatar size: " << avatarSize << " size read" << avatarDef.size();
	//qDebug() << ""; // newLine

	bool registeredSuccessfully = DB.registerUser(QString(username), QString(password));
	if (registeredSuccessfully) {
		if (!DB.changeAvatar(QString(username), avatarDef)) {
			// Remove the user. So he can make a new registration
			DB.removeUser(QString(username));
			return false;
		}
		usernames[soc->socketDescriptor()] = username;
		allUsernames[soc->socketDescriptor()] = username;
		return true;
	} else {
		return false;
	}
}

/**
 * This method sends the user's object
 * @param soc
 * @return result of writing on socket
 */
bool Server::sendUser(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - sendUser()     ---------- SEND USER ----------";
	if (soc == nullptr)
		return false;

	QByteArray message(AVATAR_MESSAGE);
	QString username = usernames[soc->socketDescriptor()];
	QByteArray image = DB.getAvatar(username);
	QByteArray usernameByteArray = convertionQString(username);
	QByteArray usernameSize = convertionNumber(usernameByteArray.size());
	QByteArray imageSize = convertionNumber(image.size());

	message.append(" " + usernameSize + " " + usernameByteArray + " " + imageSize + " " + image);

	//qDebug() << message;

	return writeMessage(soc, message);
}

/**
 * This method sends list of file names
 * @param soc
 * @return result of writing on socket
 */
bool Server::sendFileNames(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - sendFileNames()     ---------- SEND FILENAMES ----------";
	if (soc == nullptr)
		return false;

	QByteArray message(LIST_OF_FILE);
	std::map<QString, bool> files = DB.getFiles(
			allUsernames[soc->socketDescriptor()]);                  /* Get all filename associated with the relative user */
	int nFiles = files.size();
	QByteArray numFiles = convertionNumber(nFiles);

	message.append(" " + numFiles);

	for (std::pair<QString, bool> file : files) {
		QByteArray owner;
		QByteArray filenameByteArray = convertionQString(file.first);
		QByteArray fileNameSize = convertionNumber(filenameByteArray.size());
		owner.setNum(file.second ? 1 : 0);

		message.append(" " + fileNameSize + " " + filenameByteArray + " " + owner);
	}

	return writeMessage(soc, message);
}

/**
 * This method reads user's file request
 * @param socketDescriptor
 * @param soc
 * @return result of reading from socket
 */
bool Server::readFileName(qintptr socketDescriptor, QTcpSocket *soc) {
	//qDebug() << "Server.cpp - readFileName()     ---------- REQUEST FOR FILE ----------";
	if (soc == nullptr)
		return false;

	if (!readSpace(soc)) {
		return false;
	}

	/* filenameSize*/
	int filenameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	QString jsonFilename;
	if (!readQString(soc, jsonFilename, filenameSize)) {
		return false;
	}
	//qDebug() << "                               " << jsonFilename;

	QStringList fields = jsonFilename.split("%_##$$$##_%");

	if (fields.size() < 2) {
		QString owner = usernames[socketDescriptor];
		jsonFilename = owner + "%_##$$$##_%" + fields[0];
	} else {
		QString owner = fields[0];
	}

	QString username = usernames[socketDescriptor];
	QString key = jsonFilename;
	auto result = threads.find(key);

	if (result != threads.end()) {
		/* file already open */
		//qDebug() << "                               thread for file name aready exist " << jsonFilename;
		//qDebug() << ""; // newLine
		std::unique_lock<std::shared_mutex> socketsLock(result->second->mutexSockets);
		std::unique_lock<std::shared_mutex> pendingSocketsLock(result->second->mutexPendingSockets);
		std::unique_lock<std::shared_mutex> usernamesLock(result->second->mutexUsernames);
		std::shared_lock<std::shared_mutex> filenameLock(result->second->mutexFilename);

		if (!threads[key]->addSocket(soc, username)) {               /* socket transition to secondary thread */
			return false;
		}
	} else {
		std::shared_ptr<Thread> thread = addThread(key, username);
		if (!thread->addSocket(soc, usernames[socketDescriptor])) {
			return false;
		}                                                 /* socket transition to secondary thread */

		thread->moveToThread(thread.get());
		thread->start();
		//qDebug() << "                               New thread for file name: " << jsonFilename;
		//qDebug() << ""; // newLine
	}

	usernames.erase(socketDescriptor);
	return true;
}

/**
 * This methods reads user's changes from client
 * @param soc
 * @return
 */
bool Server::readEditAccount(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - readEditAccount()     ---------- READ EDIT ACCOUNT ----------";
	if (soc == nullptr) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* newUsernameSize */
	int newUsernameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* newUsername */
	QString newUsername;
	if (!readQString(soc, newUsername, newUsernameSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* newPasswordSize */
	int newPasswordSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* newPassword */
	QString newPassword;
	if (!readQString(soc, newPassword, newPasswordSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* oldPasswordSize */
	int oldPasswordSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* oldPassword */
	QString oldPassword;
	if (!readQString(soc, oldPassword, oldPasswordSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	int sizeAvatar = readNumberFromSocket(soc);
	readSpace(soc);

	//qDebug() << "                                username: " << newUsername << " size: " << newUsernameSize;
	//qDebug() << "                                password: " << newPassword << " size: " << newPasswordSize;
	//qDebug() << "                                password: " << oldPassword << " size: " << oldPasswordSize;
	//qDebug() << "                                avatar size: " << sizeAvatar;

	/* avatar */
	QByteArray avatarDef;

	if (!readChunck(soc, avatarDef, sizeAvatar)) {
		return false;
	}

	//qDebug() << "                                avatar size: " << sizeAvatar << " size read" << avatarDef.size();
	//qDebug() << ""; // newLine

	/* handle changes */
	if (DB.authenticateUser(usernames[soc->socketDescriptor()], oldPassword)) {
		if (newUsernameSize != 0) {
			if (DB.changeUsername(usernames[soc->socketDescriptor()], newUsername)) {
				QDir dir("saveData");
				dir.setNameFilters(QStringList(usernames[soc->socketDescriptor()] + "*"));
				dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

				//qDebug() << "Scanning: " << dir.path();

				QStringList fileList = dir.entryList();
				QList<std::pair<QString, QString>> renamed; // new | old  ---> when rollback rename new to old
				for (int i = 0; i < fileList.count(); i++) {
					QString filename = fileList[i].split("%_##$$$##_%")[1].split(".json")[0];
					if (fileList[i].split("%_##$$$##_%")[0] != usernames[soc->socketDescriptor()])
						continue;
					QString newFilename = newUsername + "%_##$$$##_%" + filename;
					QString oldFilename = fileList[i].split(".json")[0];
					changeNamethread(oldFilename, newFilename);
					//qDebug() << "Found file: " << fileList[i];

					renameFileSave(oldFilename, newFilename);
					/*QFile renamefile(fileList[i]);
					renamefile.rename(newFilename + ".json");
					renamefile.close();*/

					if (DB.changeFileName(oldFilename, newFilename)) {
						renamed.append(std::pair(newFilename, oldFilename));
					} else {
						// Rollback on all already renamed files!!!!
						//qDebug("Err renaming file. Doing Rollback");

						for (std::pair file : renamed) {
							/*QFile reRenameFile(f.first + ".json");
							renamefile.rename(f.second + ".json");
							renamefile.close();*/
							renameFileSave(file.first, file.second);
						}
						return false;
					}

					/* trovare tutti gli utenti che hanno in comune i file dell'utente */
					for (std::pair<qintptr, QString> username : allUsernames) {
						if (username.first != soc->socketDescriptor()) {
							std::map<QString, bool> listOfFile = DB.getFiles(username.second);

							auto result = listOfFile.find(newFilename);
							if (result != listOfFile.end()) {
								if (sockets.find(username.first) != sockets.end())
									this->sendFileNames(sockets[username.first]);
							}
						}
					}
				}

				usernames[soc->socketDescriptor()] = newUsername;
				addUsername(soc->socketDescriptor(), newUsername);
			} else {
				//qDebug() << "Err1";
				return false;
			}
		}

		if (newPasswordSize != 0) {
			if (!DB.changePassword(usernames[soc->socketDescriptor()], newPassword)) {
				//qDebug() << "Err2";
				return false;
			}
		}

		if (sizeAvatar != 0) {
			if (!DB.changeAvatar(usernames[soc->socketDescriptor()], avatarDef)) {
				//qDebug() << "Err3";
				return false;
			}
		}
	} else {
		//qDebug() << "Err4";
		return false;
	}

	return true;
}

/**
 * This method handles the disconnection from socket
 * @param soc,
 * @param connectReadyRead
 * @param connectDisconnected
 * @param socketDescriptor
 */
void Server::disconnected(QMetaObject::Connection *connectReadyRead, QMetaObject::Connection *connectDisconnected,
						  QTcpSocket *soc, qintptr socketDescriptor) {
	std::unique_lock<std::shared_mutex> allUsernamesMutex(mutexAllUsernames);
	//qDebug() << "                              " << socketDescriptor << " Disconnected (form main thread)";
	//qDebug() << ""; // newLine

	disconnect(*connectReadyRead);
	disconnect(*connectDisconnected);
	delete connectReadyRead;
	delete connectDisconnected;
	soc->deleteLater();
	try {
		socketsState.erase(socketDescriptor);
		usernames.erase(socketDescriptor);
		removeUsername(socketDescriptor);
	} catch (std::exception e) {
		//qDebug() << e.what();
	}
}

/**
 * @param fileName
 * @return Thread with a specific ID
 */
std::shared_ptr<Thread> Server::getThread(const QString &fileName) {
	auto result = threads.find(fileName);

	if (result != threads.end()) {
		return result.operator->()->second;
	} else {
		return std::shared_ptr<Thread>();
	}
}

/**
 * This method adds new Thread for specific file name
 * @param fileName
 * @return new Thread
 */
std::shared_ptr<Thread> Server::addThread(const QString &fileName, const QString &username) {
	CRDT *loadedCrdt = new CRDT();
	std::shared_ptr<Thread> thread;                        /* create new thread */

	if (!loadedCrdt->loadCRDT(fileName)) {
		//qDebug() << "File need to be created";
		CRDT *crdt = new CRDT();
		crdt->addInitialBlock();
		DB.createFile(fileName, username);
		/* create new thread */
		thread = std::make_shared<Thread>(nullptr, crdt, fileName, username, this);
	} else {
		/* create new thread */
		thread = std::make_shared<Thread>(nullptr, loadedCrdt, fileName, username, this);
	}
	threads[fileName] = thread;
	return thread;
}

/**
 * Read a shareCode from socket interact with DB
 * @param soc
 * @return bool
 */
bool Server::readShareCode(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - readShareCode()     ---------- READ SHARECODE ----------";
	if (soc == nullptr)
		return false;

	if (!readSpace(soc)) {
		return false;
	}

	/* shareCodeSize */
	int shareCodeSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* shareCode */
	QByteArray shareCode;
	if (!readChunck(soc, shareCode, shareCodeSize)) {
		return false;
	}

	//qDebug() << "                               " << shareCodeSize << shareCode;
	QString filename;

	if (handleShareCode(usernames[soc->socketDescriptor()], shareCode, filename)) {
		return sendAddFile(soc, filename);
	} else {
		return false;
	}
}

bool Server::handleShareCode(QString username, const QString &shareCode, QString &filename) {
	std::pair<QString, QString> pair = getInfoFromShareCode(shareCode);
	QString usernameOwner = pair.first;                    // TODO check problem in DB structure
	filename = pair.second;

	if (filename == "ERROR") {
		return false;
	}

	if (!DB.filenameIsValid(filename, usernameOwner))
		return false;

	return DB.addPermission(filename, usernameOwner, std::move(username));
}

/**
 * This method sends confirmation that the file has been shared.
 * @param soc
 * @param filename
 * @return bool
 */
bool Server::sendAddFile(QTcpSocket *soc, const QString &filename) {
	////qDebug() << "Server.cpp - sendAddFile()     ---------- SEND ADD FILE ----------";
	if (soc == nullptr)
		return false;

	QByteArray message(ADD_FILE);
	QByteArray filenameByteArray = convertionQString(filename);
	QByteArray fileNameSize = convertionNumber(filenameByteArray.size());
	QByteArray owner;
	owner.setNum(0);

	message.append(" " + fileNameSize + " " + filenameByteArray + " " + owner);

	return writeMessage(soc, message);
}

/**
 * Retrieve username and filename from a given shareCode <--- username + "%_##$$$##_%" + username + "%_##$$$##_%" + filename
 * separator used: "%_##$$$##_%"
 * crypto key: 0xf55f15758b7e0153
 * @param shareCode : shareCode to decrypt
 * @return : pair <Username , Filename>
 */
std::pair<QString, QString> Server::getInfoFromShareCode(const QString &shareCode) {
	SimpleCrypt crypto;
	crypto.setKey(0xf55f15758b7e0153);
	QString decrypted = crypto.decryptToString(shareCode);
	QStringList fields = decrypted.split("%_##$$$##_%");

	if (fields.size() != 3)
		return std::pair<QString, QString>("ERROR", "ERROR");
	else
		return std::pair<QString, QString>(fields[0], fields[1] + "%_##$$$##_%" + fields[2]);
}

Database Server::getDb() const {
	return DB;
}

/**
 * This method reads the request for a list of usernames for a specific file
 * @param soc
 * @return
 */
bool Server::readRequestUsernameList(QTcpSocket *soc) {
	////qDebug() << "Server.cpp - readRequestUsernameList()     ---------- REQUEST USERNAME LIST ----------";
	if (soc == nullptr)
		return false;

	if (!readSpace(soc)) {
		return false;
	}

	/* fileNameSize */
	int fileNameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* jsonFileName */
	QString jsonFileName;
	if (!readQString(soc, jsonFileName, fileNameSize)) {
		return false;
	}

	////qDebug() << "                               " << jsonFileName;

	QStringList userlist = DB.getUsers(jsonFileName);
	////qDebug() << userlist;

	int nFiles = userlist.size();
	QByteArray message(USERNAME_LIST_FOR_FILE);
	QByteArray numUsers = convertionNumber(nFiles);

	message.append(" " + numUsers);

	for (const QString &username : userlist) {
		QByteArray usernameQBytearray = convertionQString(username);
		QByteArray usernameSize = convertionNumber(usernameQBytearray.size());
		message.append(" " + usernameSize + " " + usernameQBytearray);
	}

	return writeMessage(soc, message);
}

/**
 * This method reads the file information changes
 * @param soc
 * @return
 */
bool Server::readFileInformationChanges(QTcpSocket *soc) {
	////qDebug() << "Server.cpp - readFileInformationChanges()     ---------- READ FILE INFORMATION CHANGES ----------";
	if (soc == nullptr)
		return false;

	if (!readSpace(soc)) {
		return false;
	}

	/* oldFileNameSize */
	int oldFileNameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* oldFileName */
	QString oldJsonFileName;
	if (!readQString(soc, oldJsonFileName, oldFileNameSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* newFileNameSize */
	int newFileNameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* newJsonFileName */
	QString newJsonFileName;
	if (!readQString(soc, newJsonFileName, newFileNameSize)) {
		return false;
	}
	if (!readSpace(soc)) {
		return false;
	}

	/* usernamesSize */
	int usernamesSize = readNumberFromSocket(soc);

	////qDebug() << usernamesSize;
	QStringList removedUsers;
	if (usernamesSize != 0) {
		for (int i = 0; i < usernamesSize; i++) {
			readSpace(soc);
			int usernameSize = readNumberFromSocket(soc);
			//qDebug() << usernameSize;
			readSpace(soc);
			QString username;
			if (!readQString(soc, username, usernameSize)) {
				return false;
			}
			//qDebug() << "                              username: " << username;
			DB.removePermission(oldJsonFileName, username);
			removedUsers.append(username);
		}
	}

	//qDebug() << oldJsonFileName << newJsonFileName;

	if (newFileNameSize != 0 && newJsonFileName != oldJsonFileName) {
		if (DB.changeFileName(oldJsonFileName, newJsonFileName)) {
			renameFileSave(oldJsonFileName, newJsonFileName);
			changeNamethread(oldJsonFileName, newJsonFileName);
		} else return false;

		for (std::pair<qintptr, QString> username : allUsernames) {
			if (username.first != soc->socketDescriptor()) {
				std::map<QString, bool> listOfFile = DB.getFiles(username.second);

				auto result = listOfFile.find(newJsonFileName);
				if (result != listOfFile.end()) {
					if (!sendFileNames(sockets[username.first])) {
						return false;
					}
				}
			}
		}

	}

	for (const QString &removedUsername : removedUsers) {
		for (std::pair<qintptr, QString> username : allUsernames) {
			if (username.first != soc->socketDescriptor() || removedUsername == username.second) {
				if (!sendFileNames(sockets[username.first])) {
					return false;
				}
			}
		}
	}

	auto thread = getThread(oldJsonFileName);
	if (thread != nullptr) {
		std::unique_lock<std::shared_mutex> threadSocketsMutex(thread->mutexSockets);
		std::shared_lock<std::shared_mutex> usernamesMutex(thread->mutexUsernames);
		std::unique_lock<std::shared_mutex> pendingSocketsMutex(thread->mutexPendingSockets);
		std::map<qintptr, QString> usernames = thread->getUsernames();
		std::map<qintptr, QTcpSocket *> threadSockets = thread->getSockets();

		for (const QString &removedUsername : removedUsers) {
			for (std::pair<qintptr, QString> username : usernames) {
				if (removedUsername == username.second) {
					thread->sendRemoveUser(username.second);
					thread->addPendingSocket(username.first);
					break;
				}
			}
		}
	}

	return sendFileNames(soc);
}

/**
 * This method handles the elimination of a specific file
 * @param soc
 * @return
 */
bool Server::readDeleteFile(QTcpSocket *soc) {
	//qDebug() << "Server.cpp - readDeleteFile()     ---------- READ DELETE FILE ----------";
	if (soc == nullptr)
		return false;

	if (!readSpace(soc)) {
		return false;
	}

	/* fileNameSize */
	int fileNameSize = readNumberFromSocket(soc);
	if (!readSpace(soc)) {
		return false;
	}

	/* jsonFileName */
	QString jsonFileName;
	if (!readQString(soc, jsonFileName, fileNameSize)) {
		return false;
	}
	//qDebug() << "                               " << jsonFileName;

	std::map<QString, qintptr> usersremove;
	for (std::pair<qintptr, QString> username : allUsernames) {
		if (username.first != soc->socketDescriptor()) {
			std::map<QString, bool> listOfFile = DB.getFiles(username.second);

			auto result = listOfFile.find(jsonFileName);
			if (result != listOfFile.end()) {
				//qDebug() << username.second;
				usersremove[username.second] = username.first;
			}
		}
	}

	if (!DB.deleteFile(jsonFileName)) {
		return false;
	}

	deleteFileSave(jsonFileName);

	sendFileNames(soc);

	auto thread = getThread(jsonFileName);
	if (thread != nullptr) {
		std::unique_lock<std::shared_mutex> threadSocketsMutex(thread->mutexSockets);
		std::shared_lock<std::shared_mutex> usernamesMutex(thread->mutexUsernames);
		std::unique_lock<std::shared_mutex> pendingSocketsMutex(thread->mutexPendingSockets);
		std::unique_lock<std::shared_mutex> needToSaveMutex(thread->mutexNeedToSave);
		std::unique_lock<std::shared_mutex> fileDeletedMutex(thread->mutexFileDeleted);

		auto sockets = thread->getSockets();

		for (std::pair<qintptr, QTcpSocket *> socket : sockets) {
			if (soc->socketDescriptor() != socket.first) {
				this->sendFileNames(socket.second);
			}
		}
		thread->deleteFile();
		addDeleteFileThread(jsonFileName);
	}

	for (std::pair<QString, qintptr> user : usersremove) {
		if (!sendFileNames(this->sockets[user.second])) {
			return false;
		}
	}

	return true;
}

const std::map<qintptr, QTcpSocket *> &Server::getSockets() const {
	return sockets;
}

void Server::removeUsername(qintptr socketdescriptor) {
	allUsernames.erase(socketdescriptor);
}

void Server::removeSocket(qintptr socketDescriptor) {
	sockets.erase(socketDescriptor);
}

void Server::addUsername(qintptr socketdescriptor, QString username) {
	allUsernames[socketdescriptor] = std::move(username);
}

const std::map<qintptr, QString> &Server::getUsernames() const {
	return usernames;
}

const std::map<qintptr, QString> &Server::getAllUsernames() const {
	return allUsernames;
}

/**
 * This method changes the name of a specific thread
 * @param oldFilename
 * @param newFilename
 */
void Server::changeNamethread(const QString &oldFilename, const QString &newFilename) {
	auto result = threads.find(oldFilename);

	if (result != threads.end()) {
		threads[newFilename] = result->second;
		threads.erase(oldFilename);
		std::unique_lock<std::shared_mutex> filenameLock(result->second->mutexFilename);
		result->second->changeFileName(newFilename);
	}
}

/**
 * This method adds a thread in the deleteFileThread structure
 * @param filename
 */
void Server::addDeleteFileThread(const QString &filename) {
	if (threads.find(filename) != threads.end()) {
		deleteFileThread[filename] = threads[filename];
		threads.erase(filename);
	}
}

void Server::removeDeleteFileThread(const QString &filename) {
	deleteFileThread.erase(filename);
	//qDebug() << "Thread deleted!";
}

/**
 * This method handles the elimination of thread
 * @param filename
 */
void Server::removeThread(const QString &filename) {
	std::unique_lock<std::shared_mutex> threadLok(mutexThread);
	std::unique_lock<std::shared_mutex> deletedThreadLock(mutexDeleteFileThread);
	//qDebug() << "Thread deleted 1!";
	try {
		if (deleteFileThread.find(filename) != deleteFileThread.end()) {
			deleteFileThread[filename]->quit();
			deleteFileThread[filename]->requestInterruption();
			deleteFileThread[filename]->wait();
			deleteFileThread.erase(filename);
		} else {
			threads[filename]->quit();
			threads[filename]->requestInterruption();
			threads[filename]->wait();
			threads.erase(filename);
		}
	} catch (...) {
		//qDebug() << "Problema nella distruzione del thread!";
	}
	//qDebug() << "Thread deleted!";
}

Server::~Server() {
	std::unique_lock<std::shared_mutex> threadLock(mutexThread);
	std::unique_lock<std::shared_mutex> deletedThreadLock(mutexDeleteFileThread);
	for (std::pair<QString, std::shared_ptr<Thread>> thread : threads) {
		thread.second->quit();
		thread.second->requestInterruption();
		thread.second->wait();
		threads.erase(thread.first);
		//qDebug() << "Thread deleted! in threads";
	}
	for (std::pair<QString, std::shared_ptr<Thread>> thread : deleteFileThread) {
		thread.second->quit();
		thread.second->requestInterruption();
		thread.second->wait();
		threads.erase(thread.first);
		//qDebug() << "Thread deleted! in deleteFileThread";
	}

	for (auto soc: sockets) {
		soc.second->deleteLater();
	}
}