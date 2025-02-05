//
// Created by simone on 06/08/19.
//

#ifndef TEXTEDITOR_CHARACTER_H
#define TEXTEDITOR_CHARACTER_H

#include <string>
#include <vector>
#include <QtCore/QString>
#include "Identifier.h"
#include "CharFormat.h"
#include <QJsonDocument>
#include <QtGui/QTextCharFormat>
#include <QLabel>

class Character {
public:
    Character(char value, QTextCharFormat textCharFormat, int counter, const QString &siteId, const std::vector<Identifier> &position);
	Character();
    char getValue() const;
    const QTextCharFormat &getTextCharFormat() const;
    void setTextCharFormat(const QTextCharFormat &textCharFormat);
    int getCounter() const;
    const QString &getSiteId() const;
    const std::vector<Identifier> &getPosition() const;
    int compareTo(Character otherCharacter);

	void read(const QJsonObject &json);
	void write( QJsonObject &json) const;
	QByteArray toQByteArray();
	static Character toCharacter(QJsonDocument jsonDocument);
	static CharFormat generateCharFormat(QTextCharFormat textCharFormat);

    friend bool operator==(const Character &lhs, const Character &rhs);

    friend bool operator!=(const Character &lhs, const Character &rhs);

private:
    char value;
    QTextCharFormat textCharFormat;
    int counter;
    QString siteId;
    std::vector<Identifier> position;
    QTextBlockFormat textBlockFormat;
};


#endif //TEXTEDITOR_CHARACTER_H
