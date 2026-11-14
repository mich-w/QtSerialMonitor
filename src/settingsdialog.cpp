#include "settingsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

namespace
{
    QSettings appSettings()
    {
        return QSettings(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
    }
}

AppPreferences AppPreferences::load()
{
    QSettings settings = appSettings();
    AppPreferences preferences;
    preferences.chartRefreshRate = settings.value("performance/chartRefreshRate", 30).toInt() >= 60 ? 60 : 30;
    preferences.lineQuality = settings.value("performance/lineQuality").toString() == "fast" ? LineQuality::Fast : LineQuality::Smooth;
    preferences.openGl = openGlAvailable() && settings.value("performance/openGl", false).toBool();
    return preferences;
}

void AppPreferences::save() const
{
    QSettings settings = appSettings();
    settings.setValue("performance/chartRefreshRate", chartRefreshRate);
    settings.setValue("performance/lineQuality", lineQuality == LineQuality::Fast ? "fast" : "smooth");
    settings.setValue("performance/openGl", openGl);
}

bool AppPreferences::openGlAvailable()
{
#ifdef QCUSTOMPLOT_USE_OPENGL
    return true;
#else
    return false;
#endif
}

SettingsDialog::SettingsDialog(const AppPreferences &preferences, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Settings");
    setObjectName("settingsDialog");

    // ----- Appearance ----- //
    theme = new QComboBox(this);
    theme->setObjectName("comboBoxSettingsTheme");
    for (auto mode : {ThemeManager::Mode::System, ThemeManager::Mode::Light, ThemeManager::Mode::Dark})
        theme->addItem(ThemeManager::displayName(mode), int(mode));
    theme->setCurrentIndex(theme->findData(int(ThemeManager::instance().mode())));

    auto *appearance = new QGroupBox("Appearance", this);
    auto *appearanceForm = new QFormLayout(appearance);
    appearanceForm->addRow("Theme:", theme);

    // ----- Performance ----- //
    refreshRate = new QComboBox(this);
    refreshRate->setObjectName("comboBoxSettingsRefreshRate");
    refreshRate->addItem("30 fps (default)", 30);
    refreshRate->addItem("60 fps (smoother, more CPU)", 60);
    refreshRate->setCurrentIndex(refreshRate->findData(preferences.chartRefreshRate));

    lineQuality = new QComboBox(this);
    lineQuality->setObjectName("comboBoxSettingsLineQuality");
    lineQuality->addItem("Smooth - antialiased 2 px lines", int(AppPreferences::LineQuality::Smooth));
    lineQuality->addItem("Fast - 1 px lines, for high data rates", int(AppPreferences::LineQuality::Fast));
    lineQuality->setCurrentIndex(lineQuality->findData(int(preferences.lineQuality)));

    openGl = new QCheckBox("Hardware acceleration (OpenGL, experimental)", this);
    openGl->setObjectName("checkBoxSettingsOpenGl");
    openGl->setChecked(preferences.openGl);
    if (AppPreferences::openGlAvailable())
        openGl->setToolTip("Draw the chart on the GPU: smooth lines at about half the CPU cost of Smooth rendering.\n"
                           "For the lowest CPU use, Fast line rendering without OpenGL is still quicker.\n"
                           "Falls back to normal drawing if OpenGL cannot be started.");
    else
    {
        openGl->setEnabled(false);
        openGl->setToolTip("Not available in this build (Qt OpenGL module missing).");
    }

    auto *performance = new QGroupBox("Performance", this);
    auto *performanceForm = new QFormLayout(performance);
    performanceForm->addRow("Chart refresh rate:", refreshRate);
    performanceForm->addRow("Line rendering:", lineQuality);
    performanceForm->addRow(openGl);
    auto *note = new QLabel("All received samples are recorded regardless of these settings; they only affect drawing.", performance);
    note->setWordWrap(true);
    note->setEnabled(false);
    performanceForm->addRow(note);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(appearance);
    layout->addWidget(performance);
    layout->addStretch(1);
    layout->addWidget(buttons);
    resize(460, sizeHint().height());
}

AppPreferences SettingsDialog::preferences() const
{
    AppPreferences result;
    result.chartRefreshRate = refreshRate->currentData().toInt();
    result.lineQuality = AppPreferences::LineQuality(lineQuality->currentData().toInt());
    result.openGl = openGl->isChecked();
    return result;
}

ThemeManager::Mode SettingsDialog::themeMode() const
{
    return ThemeManager::Mode(theme->currentData().toInt());
}
