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
                       const QString &header, const QString &hunk, bool isCached, QWidget *parent)
   : QFrame { parent }
   , mGit(git)
   , mCache(cache)
   , mFileName(fileName)
   , mHeader(header)
   , mHunk(hunk)
   , mIsCached(isCached)
   , mHunkView(new FileDiffView())
{
   GitQlientSettings settings;
   const auto points = settings.globalValue("FileDiffView/FontSize", 8).toInt();

   auto font = mHunkView->font();
   font.setPointSize(points);
   mHunkView->setFont(font);
   mHunkView->loadDiff(mHunk.right(mHunk.count() - mHunk.indexOf('\n') - 1));

   const auto labelTitle = new QLabel(mHunk.mid(0, mHunk.indexOf('\n')));
   font = labelTitle->font();
   font.setFamily("DejaVu Sans Mono");
   labelTitle->setFont(font);
   const auto discardBtn = new QPushButton("Discard");
   discardBtn->setObjectName("warningButton");

   const auto stageBtn = new QPushButton("Stage");
   stageBtn->setObjectName("applyActionBtn");

   const auto layout = new QGridLayout();
   layout->setContentsMargins(10, 01, 10, 10);
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
   layout->addWidget(labelTitle, 0, 1);
   layout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
   layout->addWidget(discardBtn, 0, 3);
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 4);
   layout->addWidget(stageBtn, 0, 5);
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 6);
   layout->addWidget(mHunkView, 1, 0, 1, 7);
   layout->addItem(new QSpacerItem(1, 20, QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 0);

   setLayout(layout);

   connect(discardBtn, &QPushButton::clicked, this, &HunkWidget::discardHunk);
   connect(stageBtn, &QPushButton::clicked, this, &HunkWidget::stageHunk);
}

void HunkWidget::discardHunk()
{
   const auto file = createPatchFile();

   if (file)
   {
      QScopedPointer<GitPatches> git(new GitPatches(mGit));

      const auto ret = mIsCached ? git->resetPatch(file->fileName()) : git->discardPatch(file->fileName());

      if (ret.success)
         emit hunkStaged();

      delete file;
   }
}

void HunkWidget::stageHunk()
{
   const auto file = createPatchFile();

   if (file)
   {
      QScopedPointer<GitPatches> git(new GitPatches(mGit));

      if (auto ret = git->stagePatch(file->fileName()); ret.success)
         emit hunkStaged();

      delete file;
   }
}

QTemporaryFile *HunkWidget::createPatchFile()
{
   QScopedPointer<GitPatches> git(new GitPatches(mGit));

   auto file = new QTemporaryFile(this);

   if (file->open())
   {
      const auto content = QString("%1%2").arg(mHeader, mHunk);
      file->write(content.toUtf8());
      file->close();
      return file;
   }

   return nullptr;
}
