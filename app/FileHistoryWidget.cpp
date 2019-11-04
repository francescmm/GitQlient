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
{
   mScrollArea = new QScrollArea();
   mScrollArea->setWidget(mAnotation);
   mScrollArea->setWidgetResizable(true);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->addWidget(mScrollArea);

   setLayout(layout);
}

void FileHistoryWidget::setup(const QString &fileName)
{
   delete mAnotation;
   mAnotation = nullptr;

   const auto ret = mGit->blame(fileName);

   if (ret.success)
      processBlame(ret.output.toString());
}

void FileHistoryWidget::processBlame(const QString &blame)
{
   const auto lines = blame.split("\n", QString::SkipEmptyParts);
   QVector<Annotation> annotations;
   QString file;

   mAnotation = new QFrame();
   mAnotation->setObjectName("AnnotationFrame");
   const auto annotationLayout = new QGridLayout(mAnotation);
   annotationLayout->setSpacing(0);

   QLabel *shaLabel = nullptr;
   QRect boundingRect;
   auto row = 0;
   auto labelRow = 0;
   auto labelRowSpan = 1;

   QFont f;
   f.setFamily("Ubuntu Mono");
   f.setPointSize(11);

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

      if (lastAnnotation.shortSha != a.shortSha)
      {
         if (shaLabel)
            annotationLayout->addWidget(shaLabel, labelRow, 0, labelRowSpan, 1);

         labelRow = row;
         labelRowSpan = 1;
         shaLabel = new QLabel(a.shortSha);
         shaLabel->setObjectName(row == 0 ? QString("primusInterPares")
                                          : a.shortSha == ZERO_SHA.left(8) ? QString("firstOfItsNameWIP")
                                                                           : QString("firstOfItsName"));
         shaLabel->setToolTip(a.toString());
         shaLabel->setFont(f);
         shaLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

         QFontMetrics fm(f);
         boundingRect = fm.boundingRect(a.shortSha);
      }
      else
         ++labelRowSpan;

      const auto numberLabel = new QLabel(QString::number(row + 1));
      numberLabel->setFont(f);
      numberLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
      numberLabel->setObjectName("numberLabel");
      numberLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
      annotationLayout->addWidget(numberLabel, row, 1);

      const auto contentLabel = new QLabel(content);
      contentLabel->setFont(f);
      contentLabel->setObjectName("normalLabel");
      annotationLayout->addWidget(contentLabel, row, 2);

      file.append(content + QString("\n"));

      ++row;
   }

   // Adding the last row
   if (shaLabel)
      annotationLayout->addWidget(shaLabel, labelRow, 0, labelRowSpan, 1);

   annotationLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), row, 2);

   mScrollArea->setWidget(mAnotation);
   mScrollArea->setWidgetResizable(true);
}
