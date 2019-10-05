#include "advancedgraphmenu.h"
#include "ui_advancedgraphmenu.h"

AdvancedGraphMenu::AdvancedGraphMenu(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdvancedGraphMenu)
{
    ui->setupUi(this);

    QObject::connect(ui->listWidgetLabels, SIGNAL(itemChanged(QListWidgetItem*)),
                     this, SLOT(highlightChecked(QListWidgetItem*)));
}

AdvancedGraphMenu::~AdvancedGraphMenu()
{
    delete ui;
}

void AdvancedGraphMenu::createConnections()
{
    for(int i = 0; i < ui->listWidgetLabels->count(); ++i)
    {
        ui->listWidgetLabels->item(i)->setFlags(ui->listWidgetLabels->item(i)->flags() | Qt::ItemIsUserCheckable);
    }
}

void AdvancedGraphMenu::addLabelToList(QString label)
{
    ui->listWidgetLabels->addItem(label);
    ui->listWidgetLabels->item(ui->listWidgetLabels->count() - 1)->setFlags(ui->listWidgetLabels->item(ui->listWidgetLabels->count() - 1)->flags() | Qt::ItemIsUserCheckable);
    ui->listWidgetLabels->item(ui->listWidgetLabels->count() - 1)->setCheckState(Qt::Unchecked);
}

void AdvancedGraphMenu::highlightChecked(QListWidgetItem* item)
{
    qDebug() << item->text();
}

bool AdvancedGraphMenu::searchForItem(QString label)
{
    for (auto i = 0; i < ui->listWidgetLabels->count(); ++i)
    {
        if (ui->listWidgetLabels->item(i)->text().compare(label) == 0)
            return true;
    }

    return false;
}

void AdvancedGraphMenu::clearLabelList()
{
    ui->listWidgetLabels->clear();
}

void AdvancedGraphMenu::removeLabelAt(int index)
{
    ui->listWidgetLabels->takeItem(index);
}

void AdvancedGraphMenu::on_pushButtonApply_clicked()
{
    QList<QString> labels;

    for (auto i = 0; i < ui->listWidgetLabels->count(); ++i)
    {
        if (ui->listWidgetLabels->item(i)->checkState())
            labels.append(ui->listWidgetLabels->item(i)->text());
    }

    emit readyToSyncLabelList(&labels);
}

void AdvancedGraphMenu::on_pushButtonCheckAll_clicked()
{
    for (auto i = 0; i < ui->listWidgetLabels->count(); ++i)
    {
        ui->listWidgetLabels->item(i)->setCheckState(Qt::CheckState::Checked);
    }
}

void AdvancedGraphMenu::on_pushButtonUncheckAll_clicked()
{
    for (auto i = 0; i < ui->listWidgetLabels->count(); ++i)
    {
        ui->listWidgetLabels->item(i)->setCheckState(Qt::CheckState::Unchecked);
    }
}

void AdvancedGraphMenu::on_pushButtonClear_clicked()
{
    ui->listWidgetLabels->clear();
}
