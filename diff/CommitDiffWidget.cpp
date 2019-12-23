#include "CommitDiffWidget.h"

#include <git.h>
#include <FileListWidget.h>

#include <QVBoxLayout>
#include <QLabel>

CommitDiffWidget::CommitDiffWidget(QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mFirstSha(new QLabel())
   , mSecondSha(new QLabel())
   , fileListWidget(new FileListWidget(mGit))
{
   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addWidget(new QLabel(tr("First SHA:")));
   layout->addWidget(mFirstSha);
   layout->addWidget(new QLabel(tr("Second SHA:")));
   layout->addWidget(mSecondSha);
   layout->addWidget(fileListWidget);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this, [](QListWidgetItem *) {});
}

void CommitDiffWidget::configure(const QString &firstSha, const QString &secondSha)
{
   mFirstShaStr = firstSha;
   mSecondShaStr = secondSha;
   mFirstSha->setText(mFirstShaStr);
   mSecondSha->setText(mSecondShaStr);

   fileListWidget->insertFiles(mFirstShaStr, mSecondShaStr);
}
