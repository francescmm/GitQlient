#include "FileDiffWidget.h"
#include "FileDiffView.h"
#include "FileDiffHighlighter.h"
#include "git.h"

#include <QHBoxLayout>
#include <QPushButton>

FileDiffWidget::FileDiffWidget(const QSharedPointer<Git> &git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mDiffView(new FileDiffView())
   , mGoPrevious(new QPushButton())
   , mGoNext(new QPushButton())

{
   mDiffHighlighter = new FileDiffHighlighter(mDiffView->document());

   mGoPrevious->setIcon(QIcon(":/icons/go_up"));
   mGoNext->setIcon(QIcon(":/icons/go_down"));

   const auto vLayout = new QHBoxLayout(this);
   vLayout->setContentsMargins(QMargins());
   vLayout->setSpacing(0);
   // vLayout->addLayout(controlsLayout);
   vLayout->addWidget(mDiffView);
}

void FileDiffWidget::clear()
{
   mDiffView->clear();
}

bool FileDiffWidget::configure(const QString &currentSha, const QString &previousSha, const QString &file)
{
   mCurrentFile = file;
   auto destFile = file;

   if (destFile.contains("-->"))
      destFile = destFile.split("--> ").last().split("(").first().trimmed();

   auto text = mGit->getFileDiff(currentSha == ZERO_SHA ? QString() : currentSha, previousSha, destFile);
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
