#include "macrobar.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>

MacroBar::MacroBar(QWidget *parent) : QWidget(parent)
{
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    addButton = new QToolButton(this);
    addButton->setObjectName("toolButtonAddMacro");
    addButton->setText("+");
    addButton->setToolTip("Add a macro button that sends a fixed text.");
    connect(addButton, &QToolButton::clicked, this, &MacroBar::addMacro);

    rebuild();
}

QList<MacroBar::Macro> MacroBar::macros() const
{
    return items;
}

void MacroBar::setMacros(const QList<Macro> &macros)
{
    items = macros;
    rebuild();
}

void MacroBar::load(QSettings &settings)
{
    QList<Macro> loaded;
    const int count = settings.beginReadArray("macros");
    for (auto i = 0; i < count; ++i)
    {
        settings.setArrayIndex(i);
        loaded.append({settings.value("name").toString(), settings.value("text").toString()});
    }
    settings.endArray();
    setMacros(loaded);
}

void MacroBar::save(QSettings &settings) const
{
    settings.remove("macros");
    settings.beginWriteArray("macros", items.count());
    for (auto i = 0; i < items.count(); ++i)
    {
        settings.setArrayIndex(i);
        settings.setValue("name", items[i].name);
        settings.setValue("text", items[i].text);
    }
    settings.endArray();
}

void MacroBar::rebuild()
{
    while (QLayoutItem *item = layout->takeAt(0))
    {
        if (item->widget() && item->widget() != addButton)
            item->widget()->deleteLater();
        delete item;
    }

    auto *label = new QLabel("Macros:", this);
    label->setToolTip("Click to send, right-click to edit or remove.");
    layout->addWidget(label);

    for (auto i = 0; i < items.count(); ++i)
    {
        auto *button = new QToolButton(this);
        button->setObjectName("toolButtonMacro" + QString::number(i));
        button->setText(items[i].name.isEmpty() ? items[i].text : items[i].name);
        button->setToolTip("Sends: " + items[i].text + "\nRight-click to edit or remove.");
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QToolButton::clicked, this, [this, i]()
                { emit sendRequested(items.value(i).text); });
        connect(button, &QToolButton::customContextMenuRequested, this, [this, i, button](const QPoint &pos)
                { showMacroMenu(i, button->mapToGlobal(pos)); });
        layout->addWidget(button);
    }

    layout->addWidget(addButton); // no trailing stretch: the bar only takes the room its buttons need
}

bool MacroBar::editMacro(Macro &macro, const QString &title)
{
    QDialog dialog(this);
    dialog.setWindowTitle(title);

    auto *name = new QLineEdit(macro.name, &dialog);
    name->setPlaceholderText("Button label, e.g. Reset");
    auto *text = new QLineEdit(macro.text, &dialog);
    text->setPlaceholderText("Text to send, e.g. AT+RST");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    auto *form = new QFormLayout(&dialog);
    form->addRow("Name:", name);
    form->addRow("Send:", text);
    form->addRow(new QLabel("The line ending selected next to the Send button is appended.", &dialog));
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted || text->text().isEmpty())
        return false;

    macro.name = name->text().trimmed();
    macro.text = text->text();
    return true;
}

void MacroBar::addMacro()
{
    Macro macro;
    if (editMacro(macro, "Add macro"))
    {
        items.append(macro);
        rebuild();
    }
}

void MacroBar::showMacroMenu(int index, const QPoint &globalPos)
{
    QMenu menu(this);
    QAction *edit = menu.addAction("Edit...");
    QAction *moveLeft = menu.addAction("Move left");
    moveLeft->setEnabled(index > 0);
    QAction *remove = menu.addAction("Remove");

    QAction *chosen = menu.exec(globalPos);
    if (chosen == edit)
    {
        Macro macro = items.value(index);
        if (editMacro(macro, "Edit macro"))
            items[index] = macro;
    }
    else if (chosen == moveLeft)
        items.swapItemsAt(index, index - 1);
    else if (chosen == remove)
        items.removeAt(index);
    else
        return;

    rebuild();
}
