#include "widgetsettings.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QDebug>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>

namespace
{
    QString keyPrefix(const QString &objectName)
    {
        if (objectName.startsWith("lineEdit"))
            return "data/" + objectName;
        if (objectName.startsWith("spinBox"))
            return "layout/" + objectName;
        return "GUI_Elements/" + objectName;
    }

    QWidget *findWidget(const QWidget *root, const QString &objectName)
    {
        auto *widget = root->findChild<QWidget *>(objectName);
        if (widget == nullptr)
            qWarning() << "WidgetSettings: no widget named" << objectName;
        return widget;
    }
}

void WidgetSettings::load(QSettings &settings, const QWidget *root, const QStringList &objectNames)
{
    for (const auto &name : objectNames)
    {
        QWidget *widget = findWidget(root, name);
        const QString prefix = keyPrefix(name);

        if (auto *comboBox = qobject_cast<QComboBox *>(widget))
        {
            const int index = settings.value(prefix + ".currentIndex", comboBox->currentIndex()).toInt();
            if (index >= 0 && index < comboBox->count()) // item lists may change between versions
                comboBox->setCurrentIndex(index);
        }
        else if (auto *tabWidget = qobject_cast<QTabWidget *>(widget))
        {
            const int index = settings.value(prefix + ".currentIndex", tabWidget->currentIndex()).toInt();
            if (index >= 0 && index < tabWidget->count())
                tabWidget->setCurrentIndex(index);
        }
        else if (auto *spinBox = qobject_cast<QSpinBox *>(widget))
            spinBox->setValue(settings.value(prefix + ".value", spinBox->value()).toInt());
        else if (auto *lineEdit = qobject_cast<QLineEdit *>(widget))
            lineEdit->setText(settings.value(prefix + ".text", lineEdit->text()).toString());
        else if (auto *button = qobject_cast<QAbstractButton *>(widget))
            button->setChecked(settings.value(prefix + ".isChecked", button->isChecked()).toBool());
    }
}

void WidgetSettings::save(QSettings &settings, const QWidget *root, const QStringList &objectNames)
{
    for (const auto &name : objectNames)
    {
        QWidget *widget = findWidget(root, name);
        const QString prefix = keyPrefix(name);

        if (auto *comboBox = qobject_cast<QComboBox *>(widget))
            settings.setValue(prefix + ".currentIndex", comboBox->currentIndex());
        else if (auto *tabWidget = qobject_cast<QTabWidget *>(widget))
            settings.setValue(prefix + ".currentIndex", tabWidget->currentIndex());
        else if (auto *spinBox = qobject_cast<QSpinBox *>(widget))
            settings.setValue(prefix + ".value", spinBox->value());
        else if (auto *lineEdit = qobject_cast<QLineEdit *>(widget))
            settings.setValue(prefix + ".text", lineEdit->text());
        else if (auto *button = qobject_cast<QAbstractButton *>(widget))
            settings.setValue(prefix + ".isChecked", button->isChecked());
    }
}
