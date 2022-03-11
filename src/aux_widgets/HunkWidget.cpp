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
   mHunkView->setFixedHeight(mHunkView->getHeight() + mHunkView->getLineHeigth() * 2);

   const auto labelTitle = new QLabel(mHunk.mid(0, mHunk.indexOf('\n')));
   font = labelTitle->font();
   font.setFamily("DejaVu Sans Mono");
   font.setBold(true);
   labelTitle->setFont(font);

   const auto discardBtn = new QPushButton(QString::fromUtf8(isCached ? "Unstage" : "Discard"));
   discardBtn->setObjectName("warningButton");
   connect(discardBtn, &QPushButton::clicked, this, &HunkWidget::discardHunk);

   auto controlsLayout = new QHBoxLayout();
   controlsLayout->setContentsMargins(5, 0, 5, 10);
   controlsLayout->addWidget(labelTitle);
   controlsLayout->addStretch();
   controlsLayout->addWidget(discardBtn);
   controlsLayout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed));

   if (!mIsCached)
   {
      const auto stageBtn = new QPushButton("Stage");
      stageBtn->setObjectName("applyActionBtn");
      connect(stageBtn, &QPushButton::clicked, this, &HunkWidget::stageHunk);

      controlsLayout->addWidget(stageBtn);
   }

   const auto layout = new QVBoxLayout();
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);
   layout->addLayout(controlsLayout);
   layout->addWidget(mHunkView);

   setLayout(layout);
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
      const auto content = QString("%1%2\n").arg(mHeader, mHunk);
      file->write(content.toUtf8());
      file->close();
      return file;
   }

   return nullptr;
}
