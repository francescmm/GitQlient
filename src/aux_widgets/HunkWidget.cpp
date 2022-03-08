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

   auto font = mHunkView->font();
   font.setPointSize(points);
   mHunkView->setFont(font);
   mHunkView->loadDiff(mHunk.right(mHunk.count() - mHunk.indexOf('\n')));

   const auto labelTitle = new QLabel(mHunk.mid(0, mHunk.indexOf('\n')));
   font = labelTitle->font();
   font.setFamily("DejaVu Sans Mono");
   labelTitle->setFont(font);
   const auto discardBtn = new QPushButton("Discard");
   const auto stageBtn = new QPushButton("Stage");

   const auto layout = new QGridLayout();
   layout->setContentsMargins(QMargins());
   layout->addItem(new QSpacerItem(10, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
   layout->addWidget(labelTitle, 0, 1);
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

void HunkWidget::discardHunk()
{
   auto hunkLines = mHunk.split('\n');

// Creating the dark side header line
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
   auto splitBehavior = Qt::SkipEmptyParts;
#else
   auto splitBehavior = QString::SkipEmptyParts;
#endif
   const auto headerLine = hunkLines.takeFirst();
   const auto parts = headerLine.split("@@", splitBehavior);
   const auto lineParts = parts.first().split(' ', splitBehavior);
   const auto lineA = lineParts.first().split(',', splitBehavior);
   const auto lineB = lineParts.last().split(',', splitBehavior);
   auto lines = QString("%1,%2 %3,%4").arg(lineA.first(), lineB.last(), lineB.first(), lineA.last());
   const auto heading = parts.count() > 1 && parts.last().isEmpty() ? QString::fromUtf8("") : parts.last();
   auto finalHeader = QString("@@ %1 @@%2").arg(lines, heading);

   // Creating the dark side diff
   for (auto &line : hunkLines)
   {
      if (line[0] == '+')
         line[0] = '-';
      else if (line[0] == '-')
         line[0] = '+';
   }

   hunkLines.prepend(finalHeader);
   mHunk = hunkLines.join('\n');

   stageHunk();
}

void HunkWidget::stageHunk()
{
   QScopedPointer<GitPatches> git(new GitPatches(mGit));

   auto file = new QTemporaryFile(this);

   if (file->open())
   {
      const auto content = QString("%1%2").arg(mHeader, mHunk);
      file->write(content.toUtf8());
      file->close();

      if (auto ret = git->stagePatch(file->fileName()); ret.success)
         emit hunkStaged();
   }
}
