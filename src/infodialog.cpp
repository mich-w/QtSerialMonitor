#include "infodialog.h"
#include "ui_infodialog.h"

#include <QFile>
#include <QTextBrowser>

// The resources live in a static library, so they have to be registered explicitly.
static void initResources()
{
    Q_INIT_RESOURCE(resources);
}

InfoDialog::InfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InfoDialog)
{
    ui->setupUi(this);

    // Changelog tab: CHANGELOG.md from the repository, built into the app.
    initResources();
    auto *changelog = new QTextBrowser(this);
    changelog->setObjectName("textBrowserChangelog");
    changelog->setOpenExternalLinks(true);
    QFile file(":/CHANGELOG.md");
    if (file.open(QIODevice::ReadOnly))
        changelog->setMarkdown(QString::fromUtf8(file.readAll()));
    else
        changelog->setPlainText("Changelog not available.");
    changelogTab = changelog;
    ui->tabWidget->insertTab(1, changelog, "Changelog");
}

void InfoDialog::showChangelog()
{
    ui->tabWidget->setCurrentWidget(changelogTab);
    show();
    raise();
    activateWindow();
}

InfoDialog::~InfoDialog()
{
    delete ui;
}

void InfoDialog::on_pushButtonClose_clicked()
{
    this->close();
}
