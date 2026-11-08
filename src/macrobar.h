#ifndef MACROBAR_H
#define MACROBAR_H

#include <QList>
#include <QSettings>
#include <QWidget>

class QHBoxLayout;
class QToolButton;

// Row of user-defined buttons that send a fixed text. "+" adds a macro,
// right-click a macro to edit or remove it.
class MacroBar : public QWidget
{
    Q_OBJECT
public:
    struct Macro
    {
        QString name;
        QString text;
    };

    explicit MacroBar(QWidget *parent = nullptr);

    QList<Macro> macros() const;
    void setMacros(const QList<Macro> &macros);

    void load(QSettings &settings);
    void save(QSettings &settings) const;

signals:
    void sendRequested(const QString &text);

private:
    void rebuild();
    bool editMacro(Macro &macro, const QString &title);
    void addMacro();
    void showMacroMenu(int index, const QPoint &globalPos);

    QList<Macro> items;
    QHBoxLayout *layout;
    QToolButton *addButton;
};

#endif // MACROBAR_H
