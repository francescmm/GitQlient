#include "DiffWidget.h"

#include <RevisionsCache.h>
#include <CommitInfoPanel.h>
#include <FileDiffWidget.h>
#include <FullDiffWidget.h>
#include <DiffButton.h>
#include <CommitDiffWidget.h>
#include <GitQlientSettings.h>
#include <FileEditor.h>

#include <QPinnableTabWidget.h>
#include <QLogger.h>

#include <QMessageBox>
#include <QHBoxLayout>
#include <QScrollArea>

using namespace QLogger;

DiffWidget::DiffWidget(const QSharedPointer<GitBase> git, QSharedPointer<RevisionsCache> cache, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCache(cache)
   , mInfoPanelBase(new CommitInfoPanel())
   , mInfoPanelParent(new CommitInfoPanel())
   , mCenterStackedWidget(new QPinnableTabWidget())
   , mCommitDiffWidget(new CommitDiffWidget(mGit, mCache))
   , mFileEditor(new FileEditor())
{
   setAttribute(Qt::WA_DeleteOnClose);

   mCenterStackedWidget->setCurrentIndex(0);
   mCenterStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   connect(mCenterStackedWidget, &QTabWidget::currentChanged, this, &DiffWidget::changeSelection);

   mCommitDiffWidget->setVisible(false);

   const auto diffButtonsFrame = new QFrame();
   diffButtonsFrame->setObjectName("DiffButtonsFrame");

   mDiffButtonsContainer = new QVBoxLayout(diffButtonsFrame);
   mDiffButtonsContainer->setContentsMargins(QMargins());
   mDiffButtonsContainer->setSpacing(5);
   mDiffButtonsContainer->setAlignment(Qt::AlignTop);

   const auto scrollArea = new QScrollArea();
   scrollArea->setWidget(diffButtonsFrame);
   scrollArea->setWidgetResizable(true);
   scrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

   const auto infoPanelsLayout = new QHBoxLayout();
   infoPanelsLayout->setContentsMargins(QMargins());
   infoPanelsLayout->setSpacing(10);
   infoPanelsLayout->addWidget(mInfoPanelBase);
   infoPanelsLayout->addWidget(mInfoPanelParent);

   const auto diffsLayout = new QVBoxLayout();
   diffsLayout->setContentsMargins(QMargins());
   diffsLayout->setSpacing(10);
   diffsLayout->addLayout(infoPanelsLayout);
   diffsLayout->addWidget(scrollArea);
   diffsLayout->addWidget(mCommitDiffWidget);

   mFileEditor->setVisible(false);

   const auto layout = new QHBoxLayout();
   layout->setContentsMargins(QMargins());
   layout->addLayout(diffsLayout);
   layout->setSpacing(10);
   layout->addWidget(mCenterStackedWidget);
   layout->addWidget(mFileEditor);

   setLayout(layout);

   connect(mCommitDiffWidget, &CommitDiffWidget::signalOpenFileCommit, this, &DiffWidget::loadFileDiff);
   connect(mCommitDiffWidget, &CommitDiffWidget::signalShowFileHistory, this, &DiffWidget::signalShowFileHistory);

   if (GitQlientSettings settings; !settings.value("isGitQlient", false).toBool())
      connect(mCommitDiffWidget, &CommitDiffWidget::signalEditFile, this, &DiffWidget::signalEditFile);
   else
   {
      connect(mCommitDiffWidget, &CommitDiffWidget::signalEditFile, this, &DiffWidget::startEditFile);
      connect(mFileEditor, &FileEditor::signalEditionClosed, this, &DiffWidget::endEditFile);
   }
}

DiffWidget::~DiffWidget()
{
   mDiffButtons.clear();
   blockSignals(true);
}

void DiffWidget::reload()
{
   if (mCenterStackedWidget->count() > 0)
   {
      if (const auto fileDiff = dynamic_cast<FileDiffWidget *>(mCenterStackedWidget->currentWidget()))
         fileDiff->reload();
      else if (const auto fullDiff = dynamic_cast<FullDiffWidget *>(mCenterStackedWidget->currentWidget()))
         fullDiff->reload();
   }
}

void DiffWidget::clear() const
{
   mCenterStackedWidget->setCurrentIndex(0);
}

bool DiffWidget::loadFileDiff(const QString &currentSha, const QString &previousSha, const QString &file)
{
   mFileEditor->finishEdition();

   const auto id = QString("%1 (%2 \u2194 %3)").arg(file.split("/").last(), currentSha.left(6), previousSha.left(6));

   if (!mDiffButtons.contains(id))
   {
      QLog_Info(
          "UI",
          QString("Requested diff for file {%1} on between commits {%2} and {%3}").arg(file, currentSha, previousSha));

      const auto fileDiffWidget = new FileDiffWidget(mGit, mCache);
      const auto fileWithModifications = fileDiffWidget->configure(currentSha, previousSha, file);

      if (fileWithModifications)
      {
         mInfoPanelBase->configure(mCache->getCommitInfo(currentSha));
         mInfoPanelParent->configure(mCache->getCommitInfo(previousSha));

         const auto diffButton = new DiffButton(id, ":/icons/file");
         diffButton->setSelected();

         for (const auto &buttons : qAsConst(mDiffButtons))
            buttons.second->setUnselected();

         connect(diffButton, &DiffButton::clicked, this, [this, fileDiffWidget, diffButton]() {
            mCenterStackedWidget->setCurrentWidget(fileDiffWidget);
            mCommitDiffWidget->configure(fileDiffWidget->getCurrentSha(), fileDiffWidget->getPreviousSha());

            for (const auto &buttons : qAsConst(mDiffButtons))
               if (buttons.second != diffButton)
                  buttons.second->setUnselected();
         });
         connect(diffButton, &DiffButton::destroyed, this, [this, id, fileDiffWidget]() {
            const auto index = mCenterStackedWidget->indexOf(fileDiffWidget);
            mCenterStackedWidget->removeTab(index);
            delete fileDiffWidget;
            mDiffButtons.remove(id);

            if (mDiffButtons.count() == 0)
            {
               mCommitDiffWidget->setVisible(false);
               emit signalDiffEmpty();
            }
         });

         mDiffButtonsContainer->addWidget(diffButton);
         mDiffButtons.insert(id, { fileDiffWidget, diffButton });

         const auto index = mCenterStackedWidget->addTab(fileDiffWidget, file.split("/").last());
         mCenterStackedWidget->setCurrentIndex(index);

         mCommitDiffWidget->configure(currentSha, previousSha);
         mCommitDiffWidget->setVisible(true);

         return true;
      }
      else
      {
         QMessageBox::information(this, tr("No modifications"), tr("There are no content modifications for this file"));
         delete fileDiffWidget;

         return false;
      }
   }
   else
   {
      for (const auto &buttons : qAsConst(mDiffButtons))
         buttons.second->setUnselected();

      const auto &pair = mDiffButtons.value(id);
      const auto diff = dynamic_cast<FileDiffWidget *>(pair.first);
      diff->reload();

      pair.second->setSelected();
      mCenterStackedWidget->setCurrentWidget(diff);

      return true;
   }
}

bool DiffWidget::loadCommitDiff(const QString &sha, const QString &parentSha)
{
   const auto id = QString("Commit diff (%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6));

   if (!mDiffButtons.contains(id))
   {
      const auto fullDiffWidget = new FullDiffWidget(mGit, mCache);

      if (fullDiffWidget->loadDiff(sha, parentSha))
      {
         mInfoPanelBase->configure(mCache->getCommitInfo(sha));
         mInfoPanelParent->configure(mCache->getCommitInfo(parentSha));

         const auto diffButton = new DiffButton(id, ":/icons/commit-list");
         diffButton->setSelected();

         for (const auto &buttons : qAsConst(mDiffButtons))
            buttons.second->setUnselected();

         connect(diffButton, &DiffButton::clicked, this, [this, fullDiffWidget, diffButton, sha, parentSha]() {
            mCenterStackedWidget->setCurrentWidget(fullDiffWidget);
            mCommitDiffWidget->configure(sha, parentSha);

            for (const auto &buttons : qAsConst(mDiffButtons))
               if (buttons.second != diffButton)
                  buttons.second->setUnselected();
         });
         connect(diffButton, &DiffButton::destroyed, this, [this, id, fullDiffWidget]() {
            const auto index = mCenterStackedWidget->indexOf(fullDiffWidget);
            mCenterStackedWidget->removeTab(index);
            delete fullDiffWidget;
            mDiffButtons.remove(id);

            if (mDiffButtons.count() == 0)
            {
               mCommitDiffWidget->setVisible(false);
               emit signalDiffEmpty();
            }
         });
         mDiffButtonsContainer->addWidget(diffButton);
         mDiffButtons.insert(id, { fullDiffWidget, diffButton });

         const auto index = mCenterStackedWidget->addTab(fullDiffWidget,
                                                         QString("(%1 \u2194 %2)").arg(sha.left(6), parentSha.left(6)));
         mCenterStackedWidget->setCurrentIndex(index);

         mCommitDiffWidget->configure(sha, parentSha);
         mCommitDiffWidget->setVisible(true);
      }
      else
      {
         delete fullDiffWidget;

         QMessageBox::information(this, tr("No diff to show!"),
                                  tr("There is no diff to show between commit SHAs {%1} and {%2}").arg(sha, parentSha));

         return false;
      }
   }
   else
   {
      for (const auto &buttons : qAsConst(mDiffButtons))
         buttons.second->setUnselected();

      const auto &pair = mDiffButtons.value(id);
      const auto diff = dynamic_cast<FullDiffWidget *>(pair.first);
      diff->reload();
      pair.second->setSelected();
      mCenterStackedWidget->setCurrentWidget(diff);
   }

   return true;
}

void DiffWidget::changeSelection(int index)
{
   const auto widget = qobject_cast<IDiffWidget *>(mCenterStackedWidget->widget(index));

   for (const auto &buttons : qAsConst(mDiffButtons))
   {
      if (buttons.first == widget)
         buttons.second->setSelected();
      else
         buttons.second->setUnselected();
   }

   mInfoPanelBase->configure(mCache->getCommitInfo(widget->getCurrentSha()));
   mInfoPanelParent->configure(mCache->getCommitInfo(widget->getPreviousSha()));
}

void DiffWidget::startEditFile(const QString &fileName)
{
   mCenterStackedWidget->setVisible(false);

   mFileEditor->editFile(fileName);
   mFileEditor->setVisible(true);
}

void DiffWidget::endEditFile()
{
   mCenterStackedWidget->setVisible(true);
   mFileEditor->setVisible(false);
}
