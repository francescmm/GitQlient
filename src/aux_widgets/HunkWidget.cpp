#include <HunkWidget.h>

#include <FileDiffView.h>
#include <GitCache.h>
#include <GitPatches.h>
#include <GitQlientSettings.h>
#include <LineNumberArea.h>

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryFile>

HunkWidget::HunkWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, const QString &fileName,
                       const QString &header, const QString &hunk, QWidget *parent)
   : QFrame { parent }
   , mGit(git)
   , mCache(cache)
   , mFileName(fileName)
   , mHeader(header)
   , mHunk(hunk)
   , mHunkView(new FileDiffView())
{
   GitQlientSettings settings;
   const auto points = settings.globalValue("FileDiffView/FontSize", 8).toInt();

   QFont font = mHunkView->font();
   font.setPointSize(points);
   mHunkView->setFont(font);
   mHunkView->loadDiff(mHunk.right(mHunk.count() - mHunk.indexOf('\n')));

   const auto title = mHunk.mid(0, mHunk.indexOf('\n'));
   const auto discardBtn = new QPushButton("Discard");
   const auto stageBtn = new QPushButton("Stage");

   const auto layout = new QGridLayout();
   layout->setContentsMargins(QMargins());
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
   layout->addWidget(new QLabel(title), 0, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
   layout->addWidget(discardBtn, 0, 3);
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 4);
   layout->addWidget(stageBtn, 0, 5);
   layout->addWidget(mHunkView, 1, 0, 1, 6);
   layout->addItem(new QSpacerItem(1, 20, QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 0);

   setLayout(layout);

   connect(discardBtn, &QPushButton::clicked, this, &HunkWidget::discardHunk);
   connect(stageBtn, &QPushButton::clicked, this, &HunkWidget::stageHunk);
}

void HunkWidget::discardHunk() { }

void HunkWidget::stageHunk()
{
   QScopedPointer<GitPatches> git(new GitPatches(mGit));

   auto file = new QTemporaryFile(this);

   if (file->open())
   {
      const auto content = mHeader + mHunk;
      file->write(content.toUtf8());
      file->close();

      if (auto ret = git->stagePatch(file->fileName()); ret.success)
         emit hunkStaged();
   }
}
