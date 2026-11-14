#include "theme.h"

#include <QApplication>
#include <QPalette>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>

namespace
{
    QSettings appSettings()
    {
        return QSettings(QSettings::IniFormat, QSettings::UserScope, "QtSerialMonitor", "QtSerialMonitor");
    }

    QString css(const QColor &color) { return color.name(color.alpha() < 255 ? QColor::HexArgb : QColor::HexRgb); }
}

ThemeManager &ThemeManager::instance()
{
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager()
{
    currentColors = lightColors();

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // Follow the OS switching between light and dark while in System mode.
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this]()
            {
                if (currentMode == Mode::System)
                    apply(Mode::System);
            });
#endif
}

ThemeManager::Colors ThemeManager::lightColors()
{
    Colors c;
    c.dark = false;
    c.window = QColor("#f3f4f6");
    c.panel = QColor("#f9fafb");
    c.base = QColor("#ffffff");
    c.alternateBase = QColor("#f6f8fa");
    c.button = QColor("#ffffff");
    c.text = QColor("#1f2328");
    c.mutedText = QColor("#6b7280");
    c.border = QColor("#d0d5dd");
    c.accent = QColor("#2563eb");
    c.accentText = QColor("#ffffff");
    c.chartBackground = QColor("#ffffff");
    c.chartGrid = QColor("#eceef1");
    c.chartAxis = QColor("#9ca3af");
    c.currentLine = QColor("#eef4ff");
    c.searchHighlight = QColor("#fde68a");
    c.gutter = QColor("#f3f4f6");
    c.gutterText = QColor("#9ca3af");
    c.warning = QColor("#dc2626");
    return c;
}

ThemeManager::Colors ThemeManager::darkColors()
{
    Colors c;
    c.dark = true;
    c.window = QColor("#1b1d21");
    c.panel = QColor("#212329");
    c.base = QColor("#25272d");
    c.alternateBase = QColor("#2b2e34");
    c.button = QColor("#2e3138");
    c.text = QColor("#e5e7eb");
    c.mutedText = QColor("#9aa1ac");
    c.border = QColor("#3a3e46");
    c.accent = QColor("#4f8cff");
    c.accentText = QColor("#ffffff");
    c.chartBackground = QColor("#1f2126");
    c.chartGrid = QColor("#2c2f36");
    c.chartAxis = QColor("#6b7280");
    c.currentLine = QColor("#2a3242");
    c.searchHighlight = QColor("#7c5e10");
    c.gutter = QColor("#212329");
    c.gutterText = QColor("#6b7280");
    c.warning = QColor("#f87171");
    return c;
}

bool ThemeManager::systemPrefersDark() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    const auto scheme = QGuiApplication::styleHints()->colorScheme();
    if (scheme != Qt::ColorScheme::Unknown)
        return scheme == Qt::ColorScheme::Dark;
#endif
    // Fallback: the platform's default palette.
    const QPalette platform = QApplication::style() ? QApplication::style()->standardPalette() : QPalette();
    return platform.color(QPalette::Window).lightness() < 128;
}

void ThemeManager::apply(Mode mode)
{
    currentMode = mode;
    const bool dark = mode == Mode::Dark || (mode == Mode::System && systemPrefersDark());
    currentColors = dark ? darkColors() : lightColors();
    const Colors &c = currentColors;

    QPalette palette;
    palette.setColor(QPalette::Window, c.window);
    palette.setColor(QPalette::WindowText, c.text);
    palette.setColor(QPalette::Base, c.base);
    palette.setColor(QPalette::AlternateBase, c.alternateBase);
    palette.setColor(QPalette::ToolTipBase, c.base);
    palette.setColor(QPalette::ToolTipText, c.text);
    palette.setColor(QPalette::PlaceholderText, c.mutedText);
    palette.setColor(QPalette::Text, c.text);
    palette.setColor(QPalette::Button, c.button);
    palette.setColor(QPalette::ButtonText, c.text);
    palette.setColor(QPalette::BrightText, c.warning);
    palette.setColor(QPalette::Highlight, c.accent);
    palette.setColor(QPalette::HighlightedText, c.accentText);
    palette.setColor(QPalette::Link, c.accent);
    palette.setColor(QPalette::Light, c.dark ? c.button.lighter(130) : QColor("#ffffff"));
    palette.setColor(QPalette::Midlight, c.dark ? c.button.lighter(115) : QColor("#e5e7eb"));
    palette.setColor(QPalette::Mid, c.border);
    palette.setColor(QPalette::Dark, c.dark ? QColor("#15161a") : QColor("#9ca3af"));
    palette.setColor(QPalette::Shadow, c.dark ? QColor("#000000") : QColor("#6b7280"));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, c.mutedText);
    palette.setColor(QPalette::Disabled, QPalette::Base, c.window);
    palette.setColor(QPalette::Disabled, QPalette::Button, c.window);

    QApplication::setPalette(palette);
    qApp->setStyleSheet(styleSheet());

    emit themeChanged();
}

void ThemeManager::applySaved()
{
    QSettings settings = appSettings();
    apply(fromSettingsName(settings.value("GUI_Elements/theme").toString()));
}

void ThemeManager::save() const
{
    QSettings settings = appSettings();
    settings.setValue("GUI_Elements/theme", settingsName(currentMode));
}

QColor ThemeManager::seriesColor(int index) const
{
    // Distinct, colour-blind-friendlier palette; brighter variants on dark backgrounds.
    static const QList<QColor> light = {"#2563eb", "#dc2626", "#16a34a", "#9333ea", "#ea580c",
                                        "#0891b2", "#db2777", "#65a30d", "#ca8a04", "#475569"};
    static const QList<QColor> dark = {"#60a5fa", "#f87171", "#4ade80", "#c084fc", "#fb923c",
                                       "#22d3ee", "#f472b6", "#a3e635", "#facc15", "#94a3b8"};
    const auto &colors = currentColors.dark ? dark : light;
    return colors[qAbs(index) % colors.count()];
}

QString ThemeManager::displayName(Mode mode)
{
    switch (mode)
    {
    case Mode::Light:
        return "Light";
    case Mode::Dark:
        return "Dark";
    default:
        return "System";
    }
}

QString ThemeManager::settingsName(Mode mode)
{
    return displayName(mode).toLower();
}

ThemeManager::Mode ThemeManager::fromSettingsName(const QString &name)
{
    if (name == "light")
        return Mode::Light;
    if (name == "dark")
        return Mode::Dark;
    return Mode::System;
}

QString ThemeManager::styleSheet() const
{
    const Colors &c = currentColors;
    const QColor hover = c.dark ? c.button.lighter(118) : QColor("#f3f6fb");
    const QColor pressed = c.dark ? c.button.lighter(135) : QColor("#e7edf7");
    const QColor handle = c.dark ? QColor("#4a4f59") : QColor("#c4c9d2");

    QString sheet = R"(
QFrame#textViewOptionsPanel { background: %panel; border: 1px solid %border; border-radius: 8px; }

QToolTip { color: %text; background: %base; border: 1px solid %border; padding: 4px 6px; border-radius: 4px; }

QGroupBox { border: 1px solid %border; border-radius: 8px; margin-top: 18px; padding: 6px 4px 4px 4px; background: %panel; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px; color: %muted; font-weight: 600; }

QPushButton { background: %button; border: 1px solid %border; border-radius: 6px; padding: 4px 12px; min-height: 20px; }
QPushButton:hover { background: %hover; border-color: %accent; }
QPushButton:pressed { background: %pressed; }
QPushButton:checked { background: %accent; color: %accentText; border-color: %accent; }
QPushButton:disabled { color: %muted; background: %window; border-color: %border; }

QToolBar { background: %window; border: none; border-bottom: 1px solid %border; spacing: 2px; padding: 3px 6px; }
QToolBar QToolButton { background: transparent; border: 1px solid transparent; border-radius: 6px; padding: 4px; }
QToolBar QToolButton:hover { background: %hover; border-color: %border; }
QToolBar QToolButton:checked { background: %accent; color: %accentText; }
QToolBar::separator { background: %border; width: 1px; margin: 4px 6px; }

QLineEdit, QPlainTextEdit, QTextEdit { background: %base; border: 1px solid %border; border-radius: 6px; padding: 3px 6px; selection-background-color: %accent; selection-color: %accentText; }
QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus { border-color: %accent; }
QLineEdit:disabled { background: %window; color: %muted; }

QTabWidget::pane { border: 1px solid %border; border-radius: 8px; top: -1px; background: %panel; }
QTabBar::tab { background: transparent; color: %muted; border: none; border-bottom: 2px solid transparent; padding: 6px 14px; margin-right: 2px; }
QTabBar::tab:selected { color: %text; border-bottom: 2px solid %accent; }
QTabBar::tab:hover:!selected { color: %text; }

QTableView { background: %base; alternate-background-color: %altbase; gridline-color: %grid; border: 1px solid %border; border-radius: 6px; selection-background-color: %accent; selection-color: %accentText; }
QHeaderView::section { background: %panel; color: %muted; border: none; border-bottom: 1px solid %border; border-right: 1px solid %grid; padding: 4px 6px; font-weight: 600; }
QTableCornerButton::section { background: %panel; border: none; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %handle; border-radius: 3px; min-height: 28px; }
QScrollBar::handle:horizontal { background: %handle; border-radius: 3px; min-width: 28px; }
QScrollBar::handle:hover { background: %muted; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QSplitter::handle { background: transparent; }
QSplitter::handle:hover { background: %accent; }

QMenuBar { background: %window; border-bottom: 1px solid %border; }
QMenuBar::item { padding: 4px 10px; background: transparent; border-radius: 4px; }
QMenuBar::item:selected { background: %hover; }
QMenu { background: %base; border: 1px solid %border; padding: 4px; }
QMenu::item { padding: 5px 24px 5px 20px; border-radius: 4px; }
QMenu::item:selected { background: %accent; color: %accentText; }
QMenu::separator { height: 1px; background: %border; margin: 4px 8px; }

QProgressBar { border: 1px solid %border; border-radius: 6px; background: %base; text-align: center; color: %text; }
QProgressBar::chunk { background: %accent; border-radius: 5px; }
)";

    sheet.replace("%text", css(c.text))
        .replace("%muted", css(c.mutedText))
        .replace("%base", css(c.base))
        .replace("%altbase", css(c.alternateBase))
        .replace("%panel", css(c.panel))
        .replace("%window", css(c.window))
        .replace("%button", css(c.button))
        .replace("%border", css(c.border))
        .replace("%accentText", css(c.accentText))
        .replace("%accent", css(c.accent))
        .replace("%hover", css(hover))
        .replace("%pressed", css(pressed))
        .replace("%handle", css(handle))
        .replace("%grid", css(c.chartGrid));
    return sheet;
}
