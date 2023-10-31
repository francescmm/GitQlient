#include <HunkWidget.h>

#include <FileDiffView.h>
#include <GitCache.h>
#include <GitPatches.h>
#include <GitQlientSettings.h>
#include <LineNumberArea.h>

#include <QAction>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTextBlock>

HunkWidget::HunkWidget(QSharedPointer<GitBase> git, QSharedPointer<GitCache> cache, const QString &fileName,
                       const QString &header, const QString &hunk, bool isCached, bool isEditable, QWidget *parent)
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
   mHunkView->loadDiff(mHunk.right(mHunk.length() - mHunk.indexOf('\n') - 1));
   mHunkView->setFixedHeight(mHunkView->getHeight() + mHunkView->getLineHeigth() * 2);
   connect(mHunkView, &QPlainTextEdit::customContextMenuRequested, this, &HunkWidget::showContextMenu);

   const auto labelTitle = new QLabel(mHunk.mid(0, mHunk.indexOf('\n')));
   font = labelTitle->font();
   font.setFamily("DejaVu Sans Mono");
   font.setBold(true);
   labelTitle->setFont(font);

   const auto discardBtn = new QPushButton(QString::fromUtf8(isCached ? "Unstage" : "Discard"));
   discardBtn->setObjectName("warningButton");
   connect(discardBtn, &QPushButton::clicked, this, &HunkWidget::discardHunk);

   const auto layout = new QVBoxLayout();
   layout->setContentsMargins(10, 10, 10, 10);
   layout->setAlignment(Qt::AlignTop | Qt::AlignVCenter);

   auto controlsLayout = new QHBoxLayout();
   controlsLayout->setContentsMargins(5, 0, 5, 10);
   controlsLayout->addWidget(labelTitle);

   if (isEditable)
   {
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
   }

   layout->addLayout(controlsLayout);
   layout->addWidget(mHunkView);

   setLayout(layout);
}

QTemporaryFile *HunkWidget::createPatchFile()
{
   if (const auto file = new QTemporaryFile(this); file->open())
   {
      const auto content = QString("%1%2\n").arg(mHeader, mHunk);
      file->write(content.toUtf8());
      file->close();
      return file;
   }

   return nullptr;
}

void HunkWidget::discardHunk()
{
   if (const auto file = createPatchFile())
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
   if (const auto file = createPatchFile())
   {
      QScopedPointer<GitPatches> git(new GitPatches(mGit));

      if (auto ret = git->stagePatch(file->fileName()); ret.success)
         emit hunkStaged();

      delete file;
   }
}

void HunkWidget::stageLine()
{
   auto lines = mHunk.split('\n');

   auto hunkParts = lines[0].split(' ');

   auto modParts = hunkParts[1].split(',');
   hunkParts[2] = QString("%1,%2").arg(modParts.first().replace("-", "+")).arg(modParts.last().toInt() + 1);

   lines[0] = hunkParts.join(' ');

   for (auto i = 0; i < lines.count(); ++i)
   {
      if (i == mLineToDiscard + 1)
         lines[i][0] = QChar('+');
      else if (lines[i].startsWith("+"))
      {
         lines.erase(lines.begin() + i);
         --mLineToDiscard;
         --i;
      }
      else if (lines[i].startsWith("-"))
         lines[i][0] = QChar(' ');
   }
   mHunk = lines.join('\n');

   if (auto file = createPatchFile())
   {

      QScopedPointer<GitPatches> git(new GitPatches(mGit));
      auto ret = git->stagePatch(file->fileName());

      delete file;

      if (ret.success)
         emit hunkStaged();
   }
}

void HunkWidget::discardLine()
{
   auto lines = mHunk.split('\n');

   auto hunkParts = lines[0].split(' ');

   auto modParts = hunkParts[2].split(',');
   auto oldStartLine = modParts.first();
   oldStartLine.replace("+", "-");
   hunkParts[1] = QString("%1,%2").arg(oldStartLine).arg(modParts.last().toInt());
   hunkParts[2] = QString("%1,%2").arg(modParts.first()).arg(modParts.last().toInt() - 1);

   lines[0] = hunkParts.join(' ');

   for (auto i = 0; i < lines.count(); ++i)
   {
      if (i == mLineToDiscard)
         lines[i][0] = QChar('-');
      else if (lines[i].startsWith("+"))
         lines[i][0] = QChar(' ');
      else if (lines[i].startsWith("-"))
      {
         lines.erase(lines.begin() + i);
         --i;
      }
   }
   mHunk = lines.join('\n');

   if (auto file = createPatchFile())
   {

      QScopedPointer<GitPatches> git(new GitPatches(mGit));
      auto ret = git->applyPatch(file->fileName());

      delete file;

      if (ret.success)
         emit hunkStaged();
   }
}

void HunkWidget::revertLine()
{
   auto lines = mHunk.split('\n');

   auto hunkParts = lines[0].split(' ');

   auto modParts = hunkParts[2].split(',');
   auto oldStartLine = modParts.first();
   oldStartLine.replace("+", "-");
   hunkParts[1] = QString("%1,%2").arg(oldStartLine).arg(modParts.last().toInt());
   hunkParts[2] = QString("%1,%2").arg(modParts.first()).arg(modParts.last().toInt() + 1);

   lines[0] = hunkParts.join(' ');

   for (auto i = 0; i < lines.count(); ++i)
   {
      if (i == mLineToDiscard + 1)
         lines[i][0] = QChar('+');
      else if (lines[i].startsWith("+"))
         lines[i][0] = QChar(' ');
      else if (lines[i].startsWith("-"))
      {
         lines.erase(lines.begin() + i);
         --i;
      }
   }
   mHunk = lines.join('\n');

   if (auto file = createPatchFile())
   {

      QScopedPointer<GitPatches> git(new GitPatches(mGit));
      auto ret = git->applyPatch(file->fileName());

      delete file;

      if (ret.success)
         emit hunkStaged();
   }
}

void HunkWidget::showContextMenu(const QPoint &pos)
{
   if (!mIsCached)
   {
      const auto cursor = mHunkView->cursorForPosition(pos);
      const auto blockText = cursor.block().text();
      QMenu *menu = nullptr;

      if (blockText.startsWith("+"))
      {
         mLineToDiscard = cursor.blockNumber();
         menu = new QMenu(this);

         connect(menu->addAction(tr("Stage line")), &QAction::triggered, this, &HunkWidget::stageLine);
         connect(menu->addAction(tr("Discard line")), &QAction::triggered, this, &HunkWidget::discardLine);
         menu->exec(mHunkView->viewport()->mapToGlobal(pos));
      }
      else if (blockText.startsWith("-"))
      {
         mLineToDiscard = cursor.blockNumber();

         menu = new QMenu(this);
         connect(menu->addAction(tr("Revert line")), &QAction::triggered, this, &HunkWidget::revertLine);

         menu->exec(mHunkView->viewport()->mapToGlobal(pos));
      }
   }
}
