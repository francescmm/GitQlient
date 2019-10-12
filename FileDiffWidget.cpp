#include "FileDiffWidget.h"
#include "FileDiffView.h"
#include "FileDiffHighlighter.h"
#include "git.h"

#include <QHBoxLayout>
#include <QPushButton>

FileDiffWidget::FileDiffWidget(QWidget *parent)
   : QFrame(parent)
   , mGit(Git::getInstance())
   , mDiffView(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())

{
   mDiffHighlighter = new FileDiffHighlighter(mDiffView->document());

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto controlsLayout = new QVBoxLayout();
   controlsLayout->setContentsMargins(QMargins());
   controlsLayout->addWidget(mGoPrevious);
   controlsLayout->addStretch();
   controlsLayout->addWidget(mGoNext);

   const auto vLayout = new QHBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(0);
   vLayout->addLayout(controlsLayout);
   vLayout->addWidget(mDiffView);

   connect(mGoPrevious, &QPushButton::clicked, this, &FileDiffWidget::moveToPreviousDiff);
   connect(mGoNext, &QPushButton::clicked, this, &FileDiffWidget::moveToNextDiff);
}

void FileDiffWidget::clear()
{
   mDiffView->clear();
}

bool FileDiffWidget::onFileDiffRequested(const QString &currentSha, const QString &previousSha, const QString &file)
{
   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   auto text = mGit->getDiff(currentSha, previousSha, destFile);
   auto lines = text.split("\n");

   for (auto i = 0; !lines.isEmpty() && i < 5; ++i)
      lines.takeFirst();

   if (!lines.isEmpty())
   {
      text = lines.join("\n");

      mDiffView->setPlainText(text);

      mRowIndex = 0;
      mDiffHighlighter->resetState();

      return true;
   }

   return false;
}

void FileDiffWidget::moveToPreviousDiff()
{
   /*
   if (mRowIndex > 0 && !mModifications.empty())
   {
      const auto destRow = mModifications.at(--mRowIndex);
      mDiffView->moveToRow(destRow);
   }
   */
}

void FileDiffWidget::moveToNextDiff()
{
   /*
   if (mRowIndex < mModifications.count() - 1)
   {
      const auto destRow = mModifications.at(++mRowIndex);
      mDiffView->moveToRow(destRow);
   }
   */
}
