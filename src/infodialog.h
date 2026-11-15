#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>

namespace Ui {
class InfoDialog;
}

class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InfoDialog(QWidget *parent = nullptr);
    ~InfoDialog();

    void showChangelog(); // open on the Changelog tab

private slots:
    void on_pushButtonClose_clicked();

private:
    Ui::InfoDialog *ui;
    QWidget *changelogTab = nullptr;
};

#endif // INFODIALOG_H
