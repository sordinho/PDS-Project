#ifndef CONNECTION_H
#define CONNECTION_H

#include <QMainWindow>
#include <QKeyEvent>

namespace Ui {
class Connection;
}

class Connection : public QMainWindow
{
    Q_OBJECT

public:
    explicit Connection(QWidget *parent = nullptr);
    ~Connection();

private:
    Ui::Connection *ui;
    QString address;

    void resizeEvent(QResizeEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void connectButtonClicked();

signals:
    void connectToAddress(QString host, QString port);

};

#endif // CONNECTION_H
