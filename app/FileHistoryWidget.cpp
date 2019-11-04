#include "FileHistoryWidget.h"

#include <FileDiffView.h>
#include <git.h>

#include <QGridLayout>
#include <QDateTime>
#include <QLabel>
#include <QScrollArea>
#include <QtMath>

struct Annotation
{
   QString shortSha;
   QString author;
   QDateTime dateTime;
   int line;
   QString content;

   QString toString() const
   {
      return QString("%1 - %2 - %3").arg(shortSha, author, dateTime.toString("dd/MM/yyyy hh:mm"));
   }
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
   qint64 secondsNewest = 0;
   qint64 secondsOldest = QDateTime::currentDateTime().toSecsSinceEpoch();

   for (const auto &line : lines)
   {
      const auto fields = line.split("\t");
      const auto dt = QDateTime::fromString(fields.at(2), Qt::ISODate);
      const auto lineNumAndContent = fields.at(3);
      const auto divisorChar = lineNumAndContent.indexOf(")");
      const auto lineText = lineNumAndContent.mid(0, divisorChar);
      const auto content = lineNumAndContent.mid(divisorChar + 1, lineNumAndContent.count() - lineText.count() - 1);

      annotations.append({ fields.at(0), QString(fields.at(1)).remove("("), dt, lineText.toInt(), content });

      if (fields.at(0) != ZERO_SHA)
      {
         const auto dtSinceEpoch = dt.toSecsSinceEpoch();

         if (secondsNewest < dtSinceEpoch)
            secondsNewest = dtSinceEpoch;

         if (secondsOldest > dtSinceEpoch)
            secondsOldest = dtSinceEpoch;
      }
   }

   mAnotation = new QFrame();
   mAnotation->setObjectName("AnnotationFrame");
   const auto annotationLayout = new QGridLayout(mAnotation);
   annotationLayout->setContentsMargins(QMargins());
   annotationLayout->setSpacing(0);

   auto labelRow = 0;
   auto labelRowSpan = 1;
   QLabel *authorLabel = nullptr;
   QFont f;
   f.setFamily("Ubuntu Mono");
   f.setPointSize(11);

   static const int totalColors = 8;
   static const QString colors[totalColors] = { "25, 65, 99",   "36, 95, 146",   "44, 116, 177",  "56, 136, 205",
                                                "87, 155, 213", "118, 174, 221", "150, 192, 221", "197, 220, 240" };

   const qint64 incrementSecs = (secondsNewest - secondsOldest) / (totalColors - 1);
   const auto totalAnnot = annotations.count();
   for (auto row = 0; row < totalAnnot; ++row)
   {
      const auto &lastAnnotation = row == 0 ? Annotation() : annotations.at(row - 1);

      const auto sha = annotations.at(row).shortSha;
      const auto author = annotations.at(row).author;
      const auto dt = annotations.at(row).dateTime;
      const auto content = annotations.at(row).content;

      if (lastAnnotation.shortSha != sha)
      {
         if (authorLabel)
            annotationLayout->addWidget(authorLabel, labelRow, 0, labelRowSpan, 1);

         const auto isWip = sha == ZERO_SHA.left(8);
         labelRow = row;
         labelRowSpan = 1;
         authorLabel = new QLabel(author);
         authorLabel->setObjectName(row == 0 ? QString("primusInterPares")
                                             : isWip ? QString("firstOfItsNameWIP") : QString("firstOfItsName"));
         authorLabel->setToolTip(annotations.at(row).toString());
         authorLabel->setFont(f);
         authorLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

         if (!isWip)
         {
            const auto dtSinceEpoch = dt.toSecsSinceEpoch();
            const auto colorIndex = qCeil((secondsNewest - dtSinceEpoch) / incrementSecs);
            authorLabel->setStyleSheet(QString("QLabel { border-right: 5px solid rgb(%1) }").arg(colors[colorIndex]));
         }
         else
            authorLabel->setStyleSheet("QLabel { border-right: 5px solid #D89000 }");
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
   }

   // Adding the last row
   if (authorLabel)
      annotationLayout->addWidget(authorLabel, labelRow, 0, labelRowSpan, 1);

   annotationLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), totalAnnot, 2);

   mScrollArea->setWidget(mAnotation);
   mScrollArea->setWidgetResizable(true);
}
