#include "FileHistoryWidget.h"

#include <FileDiffView.h>
#include <git.h>

#include <QGridLayout>
#include <QDateTime>
#include <QLabel>
#include <QScrollArea>

struct Annotation
{
   QString shortSha;
   QString author;
   QDateTime dateTime;
   int line;
   QString content;

   QString toString() { return QString("%1 - %2 - %3").arg(shortSha, author, dateTime.toString("dd/MM/yyyy hh:mm")); }
};

FileHistoryWidget::FileHistoryWidget(QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mAnotation(new QFrame())
   , mFileDiffView(new FileDiffView())
   , mGridLayout(new QGridLayout())
{
   mFileDiffView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   mFileDiffView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

   mGridLayout->setContentsMargins(QMargins());
   mGridLayout->setSpacing(0);
   mGridLayout->addWidget(mFileDiffView, 0, 1);

   const auto scrollWidget = new QFrame();
   scrollWidget->setLayout(mGridLayout);

   QScrollArea *scrollArea = new QScrollArea();
   scrollArea->setWidget(scrollWidget);
   scrollArea->setWidgetResizable(true);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->addWidget(scrollArea);

   setLayout(layout);
}

void FileHistoryWidget::setup(const QString &fileName)
{
   delete mAnotation;
   mAnotation = nullptr;

   const auto ret = mGit->blame(fileName);

   if (ret.success)
   {
      const auto processedText = processBlame(ret.output.toString());
      mFileDiffView->setPlainText(processedText);
   }
}

QString FileHistoryWidget::processBlame(const QString &blame)
{
   const auto lines = blame.split("\n", QString::SkipEmptyParts);
   QVector<Annotation> annotations;
   QString file;

   mAnotation = new QFrame();
   mAnotation->setObjectName("AnnotationFrame");
   const auto anotationLayout = new QVBoxLayout(mAnotation);
   anotationLayout->setSpacing(0);

   for (const auto &line : lines)
   {
      const auto fields = line.split("\t");

      auto author = fields.at(1);
      author.remove(0, 1);

      auto datetimeStr = fields.at(2).split(" ");
      datetimeStr.takeLast();

      const auto dt = QDateTime::fromString(datetimeStr.join(" "), "yyyy-MM-dd hh:mm:ss");
      const auto lineNumAndContent = fields.at(3);
      const auto divisorChar = lineNumAndContent.indexOf(")");
      const auto lineText = lineNumAndContent.mid(0, divisorChar);
      const auto content = lineNumAndContent.mid(divisorChar + 1, lineNumAndContent.count() - lineText.count() - 1);

      const auto &lastAnnotation = annotations.isEmpty() ? Annotation() : annotations.last();

      Annotation a { fields.at(0), author, std::move(dt), lineText.toInt(), content };
      annotations.append(a);

      const auto label = new QLabel(lastAnnotation.shortSha == a.shortSha ? QString() : a.shortSha);
      label->setToolTip(a.toString());
      label->setFont(mFileDiffView->font());
      QFontMetrics fm(mFileDiffView->font());
      const auto boundingRect = fm.boundingRect(a.shortSha);
      label->setFixedSize(boundingRect.width(), boundingRect.height());

      anotationLayout->addWidget(label);

      file.append(content + QString("\n"));
   }

   mGridLayout->addWidget(mAnotation, 0, 0);

   return file;
}
