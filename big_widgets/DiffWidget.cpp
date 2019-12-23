#include "DiffWidget.h"

#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <DiffButton.h>
#include <CommitDiffWidget.h>

#include <QLogger.h>

#include <QStackedWidget>
#include <QMessageBox>
#include <QHBoxLayout>

using namespace QLogger;

DiffWidget::DiffWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , centerStackedWidget(new QStackedWidget())
   , mCommitDiffWidget(new CommitDiffWidget(mGit))
{
   centerStackedWidget->setCurrentIndex(0);
   centerStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   connect(centerStackedWidget, &QStackedWidget::currentChanged, this, [this](int index) {
      const auto widget = centerStackedWidget->widget(index);

      for (const auto buttons : qAsConst(mDiffButtons))
      {
         if (buttons.first == widget)
            buttons.second->setSelected();
         else
            buttons.second->setUnselected();
      }
   });

   mDiffButtonsContainer = new QVBoxLayout();
   mDiffButtonsContainer->setContentsMargins(QMargins());
   mDiffButtonsContainer->setSpacing(5);

   mCommitDiffWidget->setVisible(false);

   const auto diffsLayout = new QVBoxLayout();
   diffsLayout->setContentsMargins(QMargins());
   diffsLayout->setSpacing(10);
   diffsLayout->addLayout(mDiffButtonsContainer);
   diffsLayout->addStretch();
   diffsLayout->addWidget(mCommitDiffWidget);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->addLayout(diffsLayout);
   layout->addWidget(centerStackedWidget);

   setLayout(layout);

   connect(mCommitDiffWidget, &CommitDiffWidget::signalOpenFileCommit, this, &DiffWidget::loadFileDiff);
}

void DiffWidget::reload()
{
   if (centerStackedWidget->count() > 0)
   {
      if (const auto fileDiff = dynamic_cast<FileDiffWidget *>(centerStackedWidget->currentWidget()))
         fileDiff->reload();
      else if (const auto fullDiff = dynamic_cast<FullDiffWidget *>(centerStackedWidget->currentWidget()))
         fullDiff->reload();
   }
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
         diffButton->setSelected();

         for (const auto buttons : qAsConst(mDiffButtons))
            buttons.second->setUnselected();

         connect(diffButton, &DiffButton::clicked, this, [this, fileDiffWidget, diffButton]() {
            centerStackedWidget->setCurrentWidget(fileDiffWidget);
            mCommitDiffWidget->configure(fileDiffWidget->getCurrentSha(), fileDiffWidget->getPreviousSha());

            for (const auto buttons : qAsConst(mDiffButtons))
               if (buttons.second != diffButton)
                  buttons.second->setUnselected();
         });
         connect(diffButton, &DiffButton::destroyed, this, [this, id, fileDiffWidget]() {
            centerStackedWidget->removeWidget(fileDiffWidget);
            delete fileDiffWidget;
            mDiffButtons.remove(id);

            if (mDiffButtons.count() == 0)
               mCommitDiffWidget->setVisible(false);
         });

         mDiffButtonsContainer->addWidget(diffButton);
         mDiffButtons.insert(id, { fileDiffWidget, diffButton });

         const auto index = centerStackedWidget->addWidget(fileDiffWidget);
         centerStackedWidget->setCurrentIndex(index);

         mCommitDiffWidget->configure(currentSha, previousSha);
         mCommitDiffWidget->setVisible(true);
      }
      else
      {
         QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
         delete fileDiffWidget;
      }
   }
   else
   {
      for (const auto buttons : qAsConst(mDiffButtons))
         buttons.second->setUnselected();

      const auto &pair = mDiffButtons.value(id);
      const auto diff = dynamic_cast<FileDiffWidget *>(pair.first);
      diff->reload();
      pair.second->setSelected();
      centerStackedWidget->setCurrentWidget(diff);
   }
}

void DiffWidget::loadCommitDiff(const QString &sha, const QString &parentSha)
{
   const auto id = QString("Commit diff (%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6));

   if (!mDiffButtons.contains(id))
   {
      const auto fullDiffWidget = new FullDiffWidget(mGit);
      fullDiffWidget->loadDiff(sha, parentSha);

      const auto diffButton = new DiffButton(id, ":/icons/commit-list");
      diffButton->setSelected();

      for (const auto buttons : qAsConst(mDiffButtons))
         buttons.second->setUnselected();

      connect(diffButton, &DiffButton::clicked, this, [this, fullDiffWidget, diffButton]() {
         centerStackedWidget->setCurrentWidget(fullDiffWidget);
         mCommitDiffWidget->configure(fullDiffWidget->getCurrentSha(), fullDiffWidget->getPreviousSha());

         for (const auto buttons : qAsConst(mDiffButtons))
            if (buttons.second != diffButton)
               buttons.second->setUnselected();
      });
      connect(diffButton, &DiffButton::destroyed, this, [this, id, fullDiffWidget]() {
         centerStackedWidget->removeWidget(fullDiffWidget);
         delete fullDiffWidget;
         mDiffButtons.remove(id);

         if (mDiffButtons.count() == 0)
            mCommitDiffWidget->setVisible(false);
      });
      mDiffButtonsContainer->addWidget(diffButton);
      mDiffButtons.insert(id, { fullDiffWidget, diffButton });

      const auto index = centerStackedWidget->addWidget(fullDiffWidget);
      centerStackedWidget->setCurrentIndex(index);

      mCommitDiffWidget->configure(sha, parentSha);
      mCommitDiffWidget->setVisible(true);
   }
   else
   {
      for (const auto buttons : qAsConst(mDiffButtons))
         buttons.second->setUnselected();

      const auto &pair = mDiffButtons.value(id);
      const auto diff = dynamic_cast<FullDiffWidget *>(pair.first);
      diff->reload();
      pair.second->setSelected();
      centerStackedWidget->setCurrentWidget(diff);
   }
}
