#include <QMessageBox>
#include "customwidget.h"
#include "ui_customwidget.h"
#include "ShareFile/sharefile.h"

CustomWidget::CustomWidget(QWidget *parent, QString filename, QString owner, bool owned, QString sharecode) :
		QWidget(parent), filename(filename), fileShareCode(sharecode),
		ui(new Ui::CustomWidget) {
	ui->setupUi(this);
	//ui->document->setPixmap(QPixmap(":/rec/img/document_2.png"));
    ui->settings->setPixmap(QPixmap(":/rec/img/menu_2.png"));
	if (owner == "You") {
		ui->share->setPixmap(QPixmap(":/rec/img/share.png"));
        ui->network->setPixmap(QPixmap(":/rec/img/user singolo.png"));
	}
	else {
		ui->network->setPixmap(QPixmap(":/rec/img/network.png"));
		ui->share->setEnabled(false);
	}
	ui->fileName->setText(filename);
	ui->owner->setText(owner);

	connect(ui->share, SIGNAL(clicked()), this, SLOT(pushSharedButton()));
}

CustomWidget::~CustomWidget() {
	delete ui;
}

void CustomWidget::pushSharedButton() {
	ShareFile *shareFile = new ShareFile(this, filename, fileShareCode);
	shareFile->show();
}
