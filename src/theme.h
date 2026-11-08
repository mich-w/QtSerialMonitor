#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QList>
#include <QObject>

// Application-wide light/dark theme: palette, style sheet and colours for custom-drawn widgets.
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum class Mode
    {
        System,
        Light,
        Dark
    };
    Q_ENUM(Mode)

    struct Colors
    {
        bool dark = false;
        QColor window, panel, base, alternateBase, button;
        QColor text, mutedText, border, accent, accentText;
        QColor chartBackground, chartGrid, chartAxis;
        QColor currentLine, searchHighlight, gutter, gutterText;
        QColor warning;
    };

    static ThemeManager &instance();

    void apply(Mode mode);
    void applySaved(); // from QSettings, before the main window is shown
    void save() const;

    Mode mode() const { return currentMode; }
    const Colors &colors() const { return currentColors; }

    // Categorical colours for chart series, tuned for the current background.
    QColor seriesColor(int index) const;

    static QString displayName(Mode mode);
    static QString settingsName(Mode mode);
    static Mode fromSettingsName(const QString &name);

signals:
    void themeChanged();

private:
    ThemeManager();
    bool systemPrefersDark() const;
    static Colors lightColors();
    static Colors darkColors();
    QString styleSheet() const;

    Mode currentMode = Mode::System;
    Colors currentColors;
};

#endif // THEME_H
