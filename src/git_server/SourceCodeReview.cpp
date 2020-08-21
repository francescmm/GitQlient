#include <SourceCodeReview.h>

#include <FileDiffView.h>

#include <QLabel>
#include <QVBoxLayout>

SourceCodeReview::SourceCodeReview(const QString &filePath, const QString &sourceCode, int commentLine, QWidget *parent)
   : QFrame(parent)
{
   auto lines = sourceCode.split("\n");
   auto hunkDescription = lines.takeFirst();
   auto hunkRemoved = false;

   if (hunkDescription.split("@@", Qt::SkipEmptyParts).count() == 1)
   {
      --commentLine;
      hunkRemoved = true;
   }

   auto linesCount = 0;
   QString summary;

   for (auto i = 0, j = lines.count() - 1; i <= 4 && j >= 0; ++i, --j)
   {
      if (!lines[j].isEmpty() && lines[j].count() > 1)
      {
         ++linesCount;
         summary.prepend(lines[j] + QString::fromUtf8("\n"));

         if (lines[j].startsWith("-"))
            --i;
      }
   }

   if (!hunkRemoved)
   {
      ++linesCount;
      summary.prepend(hunkDescription + QString::fromUtf8("\n"));
   }

   const auto diff = new FileDiffView();
   diff->setStartingLine(commentLine - linesCount + 1);
   diff->setUnifiedDiff(true);
   diff->loadDiff(summary.trimmed());
   diff->setTextInteractionFlags(Qt::NoTextInteraction);

   const auto height = diff->getLineHeight();
   diff->setFixedHeight(linesCount * height + linesCount + 1);

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->setSpacing(0);
   mainLayout->addWidget(new QLabel(filePath));
   mainLayout->addWidget(diff);
}
