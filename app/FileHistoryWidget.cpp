#include "FileHistoryWidget.h"

#include <FileDiffView.h>
#include <git.h>
#include <Revision.h>

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QtMath>

namespace
{
static const int kTotalColors = 8;
static const QString kBorderColors[kTotalColors] { "25, 65, 99",   "36, 95, 146",   "44, 116, 177",  "56, 136, 205",
                                                   "87, 155, 213", "118, 174, 221", "150, 192, 221", "197, 220, 240" };

static qint64 kSecondsNewest = 0;
static qint64 kSecondsOldest = QDateTime::currentDateTime().toSecsSinceEpoch();
static qint64 kIncrementSecs = 0;
}

FileHistoryWidget::FileHistoryWidget(QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mAnotation(new QFrame())
{
   mInfoFont.setPointSize(9);

   mCodeFont = QFont(mInfoFont);
   mCodeFont.setFamily("Ubuntu Mono");
   mCodeFont.setPointSize(10);

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
   {
      const auto annotations = processBlame(ret.output.toString());
      formatAnnotatedFile(annotations);
   }
}

QVector<FileHistoryWidget::Annotation> FileHistoryWidget::processBlame(const QString &blame)
{
   const auto lines = blame.split("\n", QString::SkipEmptyParts);
   QVector<Annotation> annotations;

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

         if (kSecondsNewest < dtSinceEpoch)
            kSecondsNewest = dtSinceEpoch;

         if (kSecondsOldest > dtSinceEpoch)
            kSecondsOldest = dtSinceEpoch;
      }
   }

   kIncrementSecs = (kSecondsNewest - kSecondsOldest) / (kTotalColors - 1);

   return annotations;
}

void FileHistoryWidget::formatAnnotatedFile(const QVector<Annotation> &annotations)
{
   auto labelRow = 0;
   auto labelRowSpan = 1;
   QLabel *dateLabel = nullptr;
   QLabel *authorLabel = nullptr;
   QLabel *messageLabel = nullptr;

   const auto annotationLayout = new QGridLayout();
   annotationLayout->setContentsMargins(QMargins());
   annotationLayout->setSpacing(0);

   const auto totalAnnot = annotations.count();
   for (auto row = 0; row < totalAnnot; ++row)
   {
      const auto &lastAnnotation = row == 0 ? Annotation() : annotations.at(row - 1);

      if (lastAnnotation.shortSha != annotations.at(row).shortSha)
      {
         if (dateLabel)
            annotationLayout->addWidget(dateLabel, labelRow, 0, labelRowSpan, 1);

         if (authorLabel)
            annotationLayout->addWidget(authorLabel, labelRow, 1, labelRowSpan, 1);

         if (messageLabel)
            annotationLayout->addWidget(messageLabel, labelRow, 2, labelRowSpan, 1);

         dateLabel = createDateLabel(annotations.at(row), row == 0);
         authorLabel = createAuthorLabel(annotations.at(row), row == 0);
         messageLabel = createMessageLabel(annotations.at(row), row == 0);

         labelRow = row;
         labelRowSpan = 1;
      }
      else
         ++labelRowSpan;

      annotationLayout->addWidget(createNumLabel(row), row, 3);
      annotationLayout->addWidget(createCodeLabel(annotations.at(row)), row, 4);
   }

   // Adding the last row
   if (dateLabel)
      annotationLayout->addWidget(dateLabel, labelRow, 0, labelRowSpan, 1);

   if (authorLabel)
      annotationLayout->addWidget(authorLabel, labelRow, 1, labelRowSpan, 1);

   if (messageLabel)
      annotationLayout->addWidget(messageLabel, labelRow, 2, labelRowSpan, 1);

   annotationLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), totalAnnot, 2);

   mAnotation = new QFrame();
   mAnotation->setObjectName("AnnotationFrame");
   mAnotation->setLayout(annotationLayout);

   mScrollArea->setWidget(mAnotation);
   mScrollArea->setWidgetResizable(true);
}

QLabel *FileHistoryWidget::createDateLabel(const Annotation &annotation, bool isFirst)
{
   auto isWip = annotation.shortSha == ZERO_SHA.left(8);
   QString when;

   if (!isWip)
   {
      const auto days = annotation.dateTime.daysTo(QDateTime::currentDateTime());
      const auto secs = annotation.dateTime.secsTo(QDateTime::currentDateTime());
      if (days > 365)
         when.append("more than 1 year ago");
      else if (days > 1)
         when.append(QString::number(days)).append(" days ago");
      else if (days == 1)
         when.append("yesterday");
      else if (secs > 3600)
         when.append(QString::number(secs / 3600)).append(" hours ago");
      else if (secs == 3600)
         when.append("1 hour ago");
      else if (secs > 60)
         when.append(QString::number(secs / 60)).append(" minutes ago");
      else if (secs == 60)
         when.append("1 minute ago");
      else
         when.append(QString::number(secs)).append(" secs ago");
   }

   const auto dateLabel = new QLabel(when);
   dateLabel->setObjectName(isFirst ? QString("authorPrimusInterPares") : QString("authorFirstOfItsName"));
   dateLabel->setToolTip(annotation.dateTime.toString("dd/MM/yyyy hh:mm"));
   dateLabel->setFont(mInfoFont);
   dateLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

   return dateLabel;
}

QLabel *FileHistoryWidget::createAuthorLabel(const Annotation &annotation, bool isFirst)
{
   const auto authorLabel = new QLabel(annotation.author);
   authorLabel->setObjectName(isFirst ? QString("authorPrimusInterPares") : QString("authorFirstOfItsName"));
   authorLabel->setFont(mInfoFont);
   authorLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

   return authorLabel;
}

QLabel *FileHistoryWidget::createMessageLabel(const Annotation &annotation, bool isFirst)
{
   auto isWip = annotation.shortSha == ZERO_SHA.left(8);
   const auto revision = mGit->revLookup(annotation.shortSha);
   const auto commitMsg = revision ? revision->shortLog().count() < 25
           ? revision->shortLog()
           : QString("%1...").arg(revision->shortLog().left(25).trimmed())
                                   : QString("WIP");

   const auto messageLabel = new QLabel(commitMsg);
   messageLabel->setObjectName(isFirst ? QString("primusInterPares") : QString("firstOfItsName"));
   messageLabel->setToolTip(QString("<p>%1</p><p>%2</p>")
                                .arg(annotation.shortSha, revision ? revision->shortLog() : QString("Local changes")));
   messageLabel->setFont(mInfoFont);
   messageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

   if (!isWip)
   {
      const auto dtSinceEpoch = annotation.dateTime.toSecsSinceEpoch();
      const auto colorIndex = qCeil((kSecondsNewest - dtSinceEpoch) / kIncrementSecs);
      messageLabel->setStyleSheet(QString("QLabel { border-right: 5px solid rgb(%1) }").arg(kBorderColors[colorIndex]));
   }
   else
      messageLabel->setStyleSheet("QLabel { border-right: 5px solid #D89000 }");

   return messageLabel;
}

QLabel *FileHistoryWidget::createNumLabel(int row)
{
   const auto numberLabel = new QLabel(QString::number(row + 1));
   numberLabel->setFont(mCodeFont);
   numberLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
   numberLabel->setObjectName("numberLabel");
   numberLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

   return numberLabel;
}

QLabel *FileHistoryWidget::createCodeLabel(const Annotation &annotation)
{
   const auto contentLabel = new QLabel(annotation.content);
   contentLabel->setFont(mCodeFont);
   contentLabel->setObjectName("normalLabel");

   return contentLabel;
}
