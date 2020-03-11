//
// Created by simone on 05/08/19.
//

#ifndef TEXTEDITOR_EDITOR_H
#define TEXTEDITOR_EDITOR_H

#include "../Controller/Controller.h"
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTextEdit>
#include <QApplication>
#include <QTextEdit>
#include <iostream>
#include <QMovie>
#include <QComboBox>
#include <QFontComboBox>
#include <QGraphicsDropShadowEffect>
#include "../../common/Constants.h"
#include <QLabel>
#include "../utils/OtherCursor.h"
#include "EditAccount/editaccount.h"


class Controller;

namespace Ui {
    class Editor;
}

class Editor : public QMainWindow {
Q_OBJECT

//TODO: Add #ifdef
//    friend class TestGui;
public:
    Editor(QString siteId, QWidget *parent = nullptr, Controller *controller = nullptr);

    void setController(Controller *controller);
    /*void insertChar(char character, QTextCharFormat charFormat, Pos pos, QString siteId);
    void changeStyle(Pos pos, const QTextCharFormat&format, QString siteId);
    QChar deleteChar(Pos pos, QString sender);*/
    void reset();
    void replaceText(const std::vector<std::vector<Character>> initialText);
    ~Editor();
    void setFilename(QString filename);
    void closeEditAccount();

    void remoteAlignmentChanged(int alignment, int blockNumber);
    void formatText(std::vector<int> styleBlocks);

    void updateOtherCursorPosition();

    void updateAlignmentPushButton();

    //void alignmentChanged(alignment_type at, int cursorPosition);

public slots:
    void on_actionNew_File_triggered();
    void on_actionShare_file_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_as_PDF_triggered();
    void on_actionLogout_triggered();
    void removeUser(QString user);
    void setUsers(QStringList user);
    void showError();
    void editAccountClicked();
    void changeUser();
    /*MULTI THREAD*/
    void insertChar(char character, QTextCharFormat charFormat, Pos pos, QString siteId);
    void changeStyle(Pos pos, const QTextCharFormat&format, QString siteId);
    void deleteChar(Pos pos, QString sender);

signals:
    void logout();
    void showFinder();
    void localInsert(QString val, QTextCharFormat textCharFormat, Pos pos);
    void totalLocalInsert(int charsAdded, QTextCursor cursor, QString chars, int position);
    void totalLocalStyleChange(int charsAdded, QTextCursor cursor, int position);
    void localDelete(Pos startPos, Pos endPos);

private slots:
    void onTextChanged(int position, int charsRemoved, int charsAdded);
    void onCursorPositionChanged();
    void textBold();
    void textUnderline();
    void textItalic();
    void textAlign(QAction *a);
    void textColor();

    void textFamily(const QString &f);
    void textSize(const QString &p);

    void colorChanged(const QColor &c);


private:
    Ui::Editor *ui;
    QTextEdit *textEdit;
    QTextDocument *textDocument;
    QTextCursor textCursor;
    int cursorPos;
    int startSelection;
    int endSelection;
    bool isRedoAvailable;
    QString siteId;
    Controller *controller;
    QStringList users;
    QMovie *loadingMovie;
    bool loadingFlag = true;
    int colorIndex;
    EditAccount *editA = nullptr;
    


public: //TODO : just to try
    QHash<QString, OtherCursor*> otherCursors;
    
private:
    std::vector<QString> colors;


    void setFormat(CharFormat charFormat);
    void undo();
    void redo();
    bool validSignal(int i, int i1, int i2);
    void resizeEvent (QResizeEvent *event);
    void restoreCursor();
    void restoreCursorSelection();
    void saveCursor();

    void setupTextActions();
    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);

    // actions
    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;
    QAction *actionAlignLeft;
    QAction *actionAlignCenter;
    QAction *actionAlignRight;
    QAction *actionAlignJustify;

    QAction *actionTextColor;

    QFontComboBox *comboFont;
    QComboBox *comboSize;

    QGraphicsDropShadowEffect *m_shadowEffect1;
    QGraphicsDropShadowEffect *m_shadowEffect2;
    QString filename;


};

#endif //TEXTEDITOR_EDITOR_H



