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

   QString summary;

   for (auto i = 0, j = lines.count() - 1; i <= 4 && i < lines.count() && j > 0; ++i, --j)
   {
      summary.prepend(lines[j] + QString::fromUtf8("\n"));

      if (lines[j].startsWith("-"))
         --i;
   }

   if (!hunkRemoved)
      summary.prepend(hunkDescription + QString::fromUtf8("\n"));

   const auto diff = new FileDiffView();
   diff->setStartingLine(commentLine - sourceCode.count("\n") + 1);
   diff->setUnifiedDiff(true);
   diff->loadDiff(summary.trimmed());
   diff->setFixedHeight(95);

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->addWidget(new QLabel(filePath));
   mainLayout->addWidget(diff);
}
