#ifndef SHOWFILES_H
#define SHOWFILES_H

#include <QListWidget>
#include <QMainWindow>
#include "../../Controller/Controller.h"
#include "EditAccount/editaccount.h"
#include "CreateFile/createfile.h"
#include "AddFile/addfile.h"
#include <QKeyEvent>
#include <QMap>
#include <QPair>

namespace Ui {
class ShowFiles;
}

class Controller;

class ShowFiles : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShowFiles(QWidget *parent = nullptr, Controller *controller = nullptr);
    ~ShowFiles();
    void addFiles(QMap<QString, bool> l);
    void addFile(QMap<QString, bool> l);
    void closeEditAccount();
    QString getFile(QString filename);
    void closeCreateFile();

public slots:
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);
    void on_actionNew_File_triggered();
    void on_actionLogout_triggered();
    void on_actionAdd_File_triggered();
    void showError();
    void editAccount();
    void changeImage();
    void closeAddFile();

signals:
    void newFile(QString filename);
    void logout();

private:
    Ui::ShowFiles *ui;
    bool newFileShown = false;
    Controller *controller = nullptr;
    CreateFile *createFile = nullptr;
    EditAccount *editAcc = nullptr;
    AddFile *addFile1 = nullptr;

    void resizeEvent(QResizeEvent *event);

    static QString getShareCode(const QString& username,const QString& filename);
    void closeEvent(QCloseEvent *event);
};

#endif // SHOWFILES_H
