#include <RevisionWidget.h>
#include <ui_RevisionWidget.h>

#include <common.h>
#include <FileListWidget.h>
#include <git.h>
#include <revsview.h>

#include <QDateTime>

RevisionWidget::RevisionWidget(QWidget *parent)
   : QWidget(parent)
   , ui(new Ui::RevisionWidget)
   , mGit(Git::getInstance())
{
   ui->setupUi(this);

   QIcon icon(":/icons/file");
   ui->labelIcon->setPixmap(icon.pixmap(15, 15));

   connect(ui->fileListWidget, &FileListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenFileCommit(mCurrentSha, mParentSha, item->text()); });
   connect(ui->fileListWidget, &FileListWidget::contextMenu, this, &RevisionWidget::signalOpenFileContextMenu);
}

RevisionWidget::~RevisionWidget()
{
   delete ui;
}

void RevisionWidget::setup(RevsView *rv)
{
   ui->fileListWidget->setup(rv);
}

void RevisionWidget::setCurrentCommitSha(const QString &sha)
{
   clear();

   mCurrentSha = sha;
   mParentSha = sha;

   if (sha != QGit::ZERO_SHA and !sha.isEmpty())
   {
      const auto currentRev = const_cast<Rev *>(mGit->revLookup(sha));

      if (currentRev)
      {
         mCurrentSha = currentRev->sha();
         mParentSha = currentRev->parent(0);

         QDateTime commitDate = QDateTime::fromSecsSinceEpoch(currentRev->authorDate().toInt());
         ui->labelSha->setText(sha);

         const auto author = currentRev->committer();
         const auto authorName = author.split("<").first();
         const auto email = author.split("<").last().split(">").first();

         ui->labelEmail->setText(email);
         ui->labelTitle->setText(currentRev->shortLog());
         ui->labelAuthor->setText(authorName);
         ui->labelDateTime->setText(commitDate.toString("dd/MM/yyyy hh:mm"));

         const auto description = currentRev->longLog().trimmed();
         ui->labelDescription->setText(description.isEmpty() ? "No description provided." : description);

         auto f = ui->labelDescription->font();
         f.setItalic(description.isEmpty());
         ui->labelDescription->setFont(f);

         const auto files = mGit->getFiles(sha, "", false, "");
         ui->fileListWidget->update(files, true);
         ui->labelModCount->setText(QString("(%1)").arg(ui->fileListWidget->count()));
      }
   }
}

QString RevisionWidget::getCurrentCommitSha() const
{
   return mCurrentSha;
}

void RevisionWidget::clear()
{
   ui->fileListWidget->clear();
   ui->labelSha->clear();
   ui->labelEmail->clear();
   ui->labelTitle->clear();
   ui->labelAuthor->clear();
   ui->labelDateTime->clear();
   ui->labelDescription->clear();
}
