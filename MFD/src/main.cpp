#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "appcontroller.h"
#include "chartpackagemanager.h"
#include "navdatamanager.h"
#include "weatherdatamanager.h"

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Experimental MFD Demo"));
    QGuiApplication::setOrganizationName(QStringLiteral("ExperimentalPFD"));

    QQmlApplicationEngine engine;
    NavDataManager navDataManager;
    AppController appController;
    ChartPackageManager chartManager;
    WeatherDataManager weatherDataManager;
    appController.setNavDataManager(&navDataManager);

    QObject::connect(&appController, &AppController::mapConfigChanged, &chartManager, [&appController, &chartManager]() {
        chartManager.setPreferredSourceKey(appController.mapBaseSource());
    });

    chartManager.setPreferredSourceKey(appController.mapBaseSource());

    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &appController);
    engine.rootContext()->setContextProperty(QStringLiteral("chartManager"), &chartManager);
    engine.rootContext()->setContextProperty(QStringLiteral("navDataManager"), &navDataManager);
    engine.rootContext()->setContextProperty(QStringLiteral("weatherDataManager"), &weatherDataManager);
    engine.addImportPath(QStringLiteral("qrc:/"));
    engine.addImportPath(QStringLiteral(":/"));
    engine.loadFromModule(QStringLiteral("ExperimentalMfd"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
