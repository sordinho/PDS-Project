#include "Controller/Controller.h"
#include "Networking/Messanger.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCommandLineParser>
#include "../common/Constants.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#if UI
    Controller controller;
#else
    #if REGISTRATION_TEST
        /* TEST: REGISTRATION FEATURE */
        Messanger messanger;
        Messanger messanger;
        messanger.connectTo("127.0.0.1", "1234");
        messanger.registration("ciao","ciao","/Users/andrea/Documents/sfondi/preview.jpeg");
    #else
        /* TEST: TEXT EDITOR */
        Messanger messanger;

        CRDT crdt{}; // model

        messanger.connectTo("127.0.0.1", "1234");
        QString username = "test2";
        messanger.logIn(username, "test2");                         /* NOTA: occorre registrare l'utente!! */

        QString siteId = username;
        crdt.setSiteId(siteId);

        Q_INIT_RESOURCE(textEditor);

        // The QApplication class manages the GUI application's control flow and main settings.
        QCoreApplication::setOrganizationName("QtProject");
        QCoreApplication::setApplicationName("text editor");
        QCoreApplication::setApplicationVersion(QT_VERSION_STR);

        QCommandLineParser parser;
        parser.setApplicationDescription(QCoreApplication::applicationName());
        parser.addHelpOption();
        parser.addVersionOption();
        parser.addPositionalArgument("file", "The file to open.");
        parser.process(app);

        // view
        Editor editor{siteId};

        // controller
        Controller controller{&crdt, &editor, &messanger};

        /*const QRect availableGeometry = QApplication::desktop()->availableGeometry(&editor);
        editor.resize(availableGeometry.width() / 2, (availableGeometry.height() * 2) / 3);
        editor.move((availableGeometry.width() - editor.width()) / 2,
                (availableGeometry.height() - editor.height()) / 2);*/

        editor.show();
    #endif
#endif
    return app.exec();
}