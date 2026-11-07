#ifndef WIDGETSETTINGS_H
#define WIDGETSETTINGS_H

#include <QSettings>
#include <QStringList>
#include <QWidget>

// Persists the state of named child widgets in QSettings.
// Keys follow the historical scheme "<group>/<objectName>.<property>", e.g.
// "GUI_Elements/checkBoxDTR.isChecked", "layout/spinBoxMaxLines.value",
// "data/lineEditSaveLogPath.text", so settings from older versions still load.
// A missing key leaves the widget at its current (default) value.
namespace WidgetSettings
{
    void load(QSettings &settings, const QWidget *root, const QStringList &objectNames);
    void save(QSettings &settings, const QWidget *root, const QStringList &objectNames);
}

#endif // WIDGETSETTINGS_H
