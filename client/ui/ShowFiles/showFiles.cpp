#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include "showFiles.h"
#include "ui_showFiles.h"
#include "../../Networking/Messanger.h"
#include "CustomWidget/customwidget.h"
#include "../../../server/SimpleCrypt/SimpleCrypt.h"
#include "AddFile/addfile.h"

ShowFiles::ShowFiles(QWidget *parent, Controller *controller) :
		QMainWindow(parent),
		controller(controller),
		ui(new Ui::ShowFiles) {
	ui->setupUi(this);

	connect(this->controller, SIGNAL(userRecived()), this, SLOT(changeImage()));

	ui->newFile->setPixmap(QPixmap(":/rec/img/new-file.png"));
	ui->addFile->setPixmap(QPixmap(":/rec/img/addfile.png"));
	ui->logout->setPixmap(QPixmap(":/rec/img/logout.png"));
	if (controller->getUser() != nullptr)
	    changeImage();

	connect(ui->newFile, SIGNAL(clicked()), this, SLOT(on_actionNew_File_triggered()));
	connect(ui->addFile, SIGNAL(clicked()), this, SLOT(on_actionAdd_File_triggered()));
	connect(ui->avatar, SIGNAL(clicked()), this, SLOT(editAccount()));
	connect(ui->logout, SIGNAL(clicked()), this, SLOT(on_actionLogout_triggered()));

	QGraphicsDropShadowEffect *m_shadowEffect = new QGraphicsDropShadowEffect(this);
	m_shadowEffect->setColor(QColor(0, 0, 0, 255 * 0.1));
	m_shadowEffect->setXOffset(0);
	m_shadowEffect->setYOffset(4);
	m_shadowEffect->setBlurRadius(12);
    // hide shadow
	m_shadowEffect->setEnabled(true);
	ui->customToolbar->setGraphicsEffect(m_shadowEffect);

}

ShowFiles::~ShowFiles() {
	delete ui;
}

void ShowFiles::on_listWidget_itemDoubleClicked(QListWidgetItem *item) {
	QString filename = item->text();
	controller->startLoadingPopup(true);
//	emit newFile(filename);
	QMetaObject::invokeMethod(controller, "requestForFile", Qt::QueuedConnection, Q_ARG(QString,filename));

}

void ShowFiles::addFiles(QMap<QString, bool> l) {
	this->ui->listWidget->clear();

	for (auto filename : l.keys()) {
		QString shareCode = "ERROR";
		// If user is owner for that file create a sharecode
		if (l[filename]) {
			QString username = controller->getUser()->getUsername();
			//qDebug() << username << filename.first;
			shareCode = getShareCode(username, filename);
		}

		QString fname = filename.split("%_##$$$##_%")[1];
        QString owner = filename.split("%_##$$$##_%")[0];
		CustomWidget *myItem = new CustomWidget(this, fname, owner,l[filename], shareCode, this->controller);
		QListWidgetItem *item = new QListWidgetItem(filename);
		item->setSizeHint(QSize(0, 60));
		this->ui->listWidget->addItem(item);
		this->ui->listWidget->setItemWidget(item, myItem);
	}
}

void ShowFiles::addFile(QMap<QString, bool> l) {
	for (auto filename : l.keys()) {
		QString shareCode = "ERROR";
		// If user is owner for that file create a sharecode
		if (l[filename]) {
			QString username = controller->getUser()->getUsername();
			shareCode = getShareCode(username, filename);
		}

        QString fname = filename.split("%_##$$$##_%")[1];
        QString owner = filename.split("%_##$$$##_%")[0] == controller->getUser()->getUsername()? "You" : filename.split("%_##$$$##_%")[0];
        CustomWidget *myItem = new CustomWidget(this, fname, owner, l[filename], shareCode, this->controller);
		QListWidgetItem *item = new QListWidgetItem(filename);
		item->setSizeHint(QSize(0, 60));
		this->ui->listWidget->addItem(item);
		this->ui->listWidget->setItemWidget(item, myItem);
	}
}

void ShowFiles::on_actionNew_File_triggered() {
	createFile = new CreateFile(controller->getGui());
	connect(createFile, SIGNAL(createFile(QString)), controller, SLOT(requestForFile(QString)));
	createFile->show();
}

void ShowFiles::on_actionLogout_triggered() {
	QMessageBox::information(this, "Logout", "Logout!");
	emit logout();
}

void ShowFiles::resizeEvent(QResizeEvent *event) {
	ui->customToolbar->setGeometry(0, 0, width(), 60);
	ui->avatar->setGeometry(width() - 50, 10, 40, 40);
    ui->logout->setGeometry(width() - 95, 14, 35, 35);
    int centralWidgetX = 0;


    if (width() > ui->listWidget->width()){
        centralWidgetX = width()/2 - ui->listWidget->width()/2;
    }else{
        centralWidgetX = 0;
    }

    ui->listWidget->setGeometry(centralWidgetX, 65, ui->listWidget->width(), height()-65);
}


void ShowFiles::showError() {
	QMessageBox::information(this, "Error", "Error for this request!");
}

void ShowFiles::editAccount() {
	this->editAcc = new EditAccount(controller->getGui(), controller->getUser());
	connect(editAcc, SIGNAL(edit(QString, QString, QString, QByteArray)), controller,
			SLOT(sendEditAccount(QString, QString, QString, QByteArray)));
	editAcc->show();
}

void ShowFiles::closeEditAccount() {
    if (this->editAcc != nullptr){
        editAcc->setParent(controller->getGui()->centralWidget());
        editAcc->close();
    }
}

/**
 * Generate the unique share code for a given file. ---> username + "%_##$$$##_%" + filename
 * separator used: "%_##$$$##_%"
 * crypto key: 0xf55f15758b7e0153
 * @param username : owner of the file to share
 * @param filename : name of the file to share
 * @return
 */
QString ShowFiles::getShareCode(const QString &username, const QString &filename) {
	SimpleCrypt crypto;
	crypto.setKey(0xf55f15758b7e0153); // Random generated key, same must be used server side!!!

	qDebug()<< "username: "+ username;
	qDebug()<< "filename: "+ filename;
	QString shareCode = crypto.encryptToString(username + "%_##$$$##_%" + filename);
	qDebug()<< "shareCode: "+ shareCode;
	return shareCode;
}

void ShowFiles::on_actionAdd_File_triggered(){
	addFile1 = new AddFile(controller->getGui());
	connect(addFile1, SIGNAL(sendShareCode(QString)), controller, SLOT(sendShareCode(QString)));
    addFile1->show();
}

void ShowFiles::closeAddFile(){
    if (addFile1 != nullptr){
        addFile1->close();
    }
}

void ShowFiles::closeCreateFile(){
    if (createFile != nullptr){
        createFile->setParent(controller->getGui()->centralWidget());
        createFile->close();
    }
}

void ShowFiles::closeEvent(QCloseEvent *event){
	if (createFile != nullptr){
		createFile->close();
	}
}

void ShowFiles::changeImage(){
    QPixmap pix = controller->getUser()->getAvatar();
    int w = pix.width();
    int h = pix.height();
    ui->avatar->setPixmap(pix.scaled(w, h, Qt::KeepAspectRatio));
}