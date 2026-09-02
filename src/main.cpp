#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlEngine>

#include "controller/AppController.h"
#include "logic/Logic.h"
#include "model/UnitInfo.h"
#include "view/Dialogs.h"
#include "view/ViewState.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QLogueLibrarian"));
    app.setOrganizationName(QStringLiteral("qlogue"));

    // UnitInfo/UnitParam cross the QML boundary as value types. Registering
    // their meta-objects lets the engine read sub-properties (e.g.
    // `library[i].name`) directly, so Logic needs no per-field "meta*"
    // properties and a unit travels as one encapsulated value.
    qmlRegisterUncreatableMetaObject(qlogue::UnitInfo::staticMetaObject,
                                     "qlogue", 1, 0, "UnitInfo",
                                     QStringLiteral("Read-only value type"));
    qmlRegisterUncreatableMetaObject(qlogue::UnitParam::staticMetaObject,
                                     "qlogue", 1, 0, "UnitParam",
                                     QStringLiteral("Read-only value type"));

    qlogue::Logic logic;
    qlogue::AppController controller(&logic);
    qlogue::ViewState viewState;   // view owns selection / presentation state
    qlogue::Dialogs dialogs;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("App"), &logic);
    engine.rootContext()->setContextProperty(QStringLiteral("Controller"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("ViewState"), &viewState);
    engine.rootContext()->setContextProperty(QStringLiteral("Dialogs"), &dialogs);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
