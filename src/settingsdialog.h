#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "theme.h"

#include <QDialog>

class QComboBox;

// Application preferences that are not part of the main window's controls.
struct AppPreferences
{
    enum class LineQuality
    {
        Smooth, // 2 px, antialiased
        Fast    // 1 px, no antialiasing - much cheaper with many points
    };

    int chartRefreshRate = 30; // frames per second: 30 or 60
    LineQuality lineQuality = LineQuality::Smooth;

    static AppPreferences load();
    void save() const;
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(const AppPreferences &preferences, QWidget *parent = nullptr);

    AppPreferences preferences() const;
    ThemeManager::Mode themeMode() const;

private:
    QComboBox *theme;
    QComboBox *refreshRate;
    QComboBox *lineQuality;
};

#endif // SETTINGSDIALOG_H
