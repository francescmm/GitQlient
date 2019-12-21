#include "DiffWidget.h"

#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <DiffButton.h>

#include <QLogger.h>

#include <QStackedWidget>
#include <QMessageBox>
#include <QHBoxLayout>

using namespace QLogger;

DiffWidget::DiffWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , centerStackedWidget(new QStackedWidget())
   , mFullDiffWidget(new FullDiffWidget(git))
   , mFileDiffWidget(new FileDiffWidget(git))
{
   centerStackedWidget->setCurrentIndex(0);
   centerStackedWidget->addWidget(mFullDiffWidget);
   centerStackedWidget->addWidget(mFileDiffWidget);
   centerStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   mDiffButtonsContainer = new QVBoxLayout();
   mDiffButtonsContainer->setContentsMargins(QMargins());
   mDiffButtonsContainer->setSpacing(5);

   const auto diffsLayout = new QVBoxLayout();
   diffsLayout->setContentsMargins(QMargins());
   diffsLayout->setSpacing(10);
   diffsLayout->addLayout(mDiffButtonsContainer);
   diffsLayout->addStretch();

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->addLayout(diffsLayout);
   layout->addWidget(centerStackedWidget);

   setLayout(layout);
}

void DiffWidget::clear() const
{
   centerStackedWidget->setCurrentIndex(0);

   /*
   for (auto values : mDiffButtons)
   {
      if (dynamic_cast<FileDiffWidget *>(values.first))
         dynamic_cast<FileDiffWidget *>(values.first)->clear();
      else if (dynamic_cast<FullDiffWidget *>(values.first))
         dynamic_cast<FullDiffWidget *>(values.first)->clear();
   }
   */

   mFullDiffWidget->clear();
   mFileDiffWidget->clear();
}

void DiffWidget::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   const auto id = QString("%1 (%2 \u2194 %3)").arg(file.split("/").last(), currentSha.left(6), previousSha.left(6));

   if (!mDiffButtons.contains(id))
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      const auto fileDiffWidget = new FileDiffWidget(mGit);
      const auto fileWithModifications = fileDiffWidget->configure(currentSha, previousSha, file);

      if (fileWithModifications)
      {
         const auto diffButton = new DiffButton(id, ":/icons/file");
         connect(diffButton, &DiffButton::clicked, this, []() {});
         connect(diffButton, &DiffButton::destroyed, []() {});

         mDiffButtonsContainer->addWidget(diffButton);
         mDiffButtons.insert(id, { fileDiffWidget, diffButton });

         const auto index = centerStackedWidget->addWidget(fileDiffWidget);
         centerStackedWidget->setCurrentIndex(index);
      }
      else
         delete fileDiffWidget;
   }
   else
      QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
}

void DiffWidget::loadCommitDiff(const QString &sha, const QString &parentSha)
{
   const auto id = QString("Complete (%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6));

   if (!mDiffButtons.contains(id))
   {
      const auto fullDiffWidget = new FullDiffWidget(mGit);
      fullDiffWidget->loadDiff(sha, parentSha);

      const auto diffButton = new DiffButton(id, ":/icons/commit-list");
      connect(diffButton, &DiffButton::clicked, this, []() {});
      connect(diffButton, &DiffButton::destroyed, []() {});
      mDiffButtonsContainer->addWidget(diffButton);
      mDiffButtons.insert(id, { fullDiffWidget, diffButton });

      const auto index = centerStackedWidget->addWidget(fullDiffWidget);
      centerStackedWidget->setCurrentIndex(index);
   }
}
