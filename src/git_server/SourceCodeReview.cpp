#include <SourceCodeReview.h>

#include <FileDiffView.h>

#include <QLabel>
#include <QVBoxLayout>

SourceCodeReview::SourceCodeReview(const QString &filePath, const QString &sourceCode, int commentLine, QWidget *parent)
   : QFrame(parent)
{
   (void)commentLine;
   auto lines = sourceCode.split("\n");
   auto hunkDescription = lines.first();

   if (hunkDescription.split("@@", Qt::SkipEmptyParts).count() == 1)
      lines.takeFirst();

   const auto diff = new FileDiffView();
   diff->setStartingLine(commentLine - sourceCode.count("\n") + 1);
   diff->setUnifiedDiff(true);
   diff->loadDiff(lines.join("\n"));
   diff->setFixedHeight(95);

   const auto mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(QMargins());
   mainLayout->addWidget(new QLabel(filePath));
   mainLayout->addWidget(diff);
}
