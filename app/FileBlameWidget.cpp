#include "FileBlameWidget.h"

#include <FileDiffView.h>
#include <git.h>
#include <Revision.h>
#include <ClickableFrame.h>

#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QtMath>
#include <QMessageBox>

namespace
{
static const int kTotalColors = 8;
static const QString kBorderColors[kTotalColors] { "25, 65, 99",   "36, 95, 146",   "44, 116, 177",  "56, 136, 205",
                                                   "87, 155, 213", "118, 174, 221", "150, 192, 221", "197, 220, 240" };

static qint64 kSecondsNewest = 0;
static qint64 kSecondsOldest = QDateTime::currentDateTime().toSecsSinceEpoch();
static qint64 kIncrementSecs = 0;
}

FileBlameWidget::FileBlameWidget(QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mAnotation(new QFrame())
{
   mAnotation->setObjectName("AnnotationFrame");

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

void FileBlameWidget::setup(const QString &fileName)
{
   const auto ret = mGit->blame(fileName);

   if (ret.success && !ret.output.toString().contains("fatal"))
   {
      delete mAnotation;
      mAnotation = nullptr;

      const auto annotations = processBlame(ret.output.toString());
      formatAnnotatedFile(annotations);
   }
   else
      QMessageBox::warning(this, tr("File not in Git"),
                           tr("The file {%1} is not under Git control version. You cannot blame it.").arg(fileName));
}

QVector<FileBlameWidget::Annotation> FileBlameWidget::processBlame(const QString &blame)
{
   const auto lines = blame.split("\n", QString::SkipEmptyParts);
   QVector<Annotation> annotations;

   for (const auto &line : lines)
   {
      const auto fields = line.split("\t");
      const auto revision = mGit->revLookup(fields.at(0));
      const auto dt = QDateTime::fromString(fields.at(2), Qt::ISODate);
      const auto lineNumAndContent = fields.at(3);
      const auto divisorChar = lineNumAndContent.indexOf(")");
      const auto lineText = lineNumAndContent.mid(0, divisorChar);
      const auto content = lineNumAndContent.mid(divisorChar + 1, lineNumAndContent.count() - lineText.count() - 1);

      annotations.append({ revision->sha(), QString(fields.at(1)).remove("("), dt, lineText.toInt(), content });

      if (fields.at(0) != ZERO_SHA)
      {
         const auto dtSinceEpoch = dt.toSecsSinceEpoch();

         if (kSecondsNewest < dtSinceEpoch)
            kSecondsNewest = dtSinceEpoch;

         if (kSecondsOldest > dtSinceEpoch)
            kSecondsOldest = dtSinceEpoch;
      }
   }

   kIncrementSecs = kSecondsNewest != kSecondsOldest ? (kSecondsNewest - kSecondsOldest) / (kTotalColors - 1) : 1;

   return annotations;
}

void FileBlameWidget::formatAnnotatedFile(const QVector<Annotation> &annotations)
{
   auto labelRow = 0;
   auto labelRowSpan = 1;
   QLabel *dateLabel = nullptr;
   QLabel *authorLabel = nullptr;
   ClickableFrame *messageLabel = nullptr;

   const auto annotationLayout = new QGridLayout();
   annotationLayout->setContentsMargins(QMargins());
   annotationLayout->setSpacing(0);

   const auto totalAnnot = annotations.count();
   for (auto row = 0; row < totalAnnot; ++row)
   {
      const auto &lastAnnotation = row == 0 ? Annotation() : annotations.at(row - 1);

      if (lastAnnotation.sha != annotations.at(row).sha)
      {
         if (dateLabel)
            annotationLayout->addWidget(dateLabel, labelRow, 0, 1, 1);

         if (authorLabel)
            annotationLayout->addWidget(authorLabel, labelRow, 1, 1, 1);

         if (messageLabel)
            annotationLayout->addWidget(messageLabel, labelRow, 2, 1, 1);

         dateLabel = createDateLabel(annotations.at(row), row == 0);
         authorLabel = createAuthorLabel(annotations.at(row), row == 0);
         messageLabel = createMessageLabel(annotations.at(row), row == 0);

         labelRow = row;
         labelRowSpan = 1;
      }
      else
         ++labelRowSpan;

      annotationLayout->addWidget(createNumLabel(annotations.at(row), row), row, 3);
      annotationLayout->addWidget(createCodeLabel(annotations.at(row)), row, 4);
   }

   // Adding the last row
   if (dateLabel)
      annotationLayout->addWidget(dateLabel, labelRow, 0, 1, 1);

   if (authorLabel)
      annotationLayout->addWidget(authorLabel, labelRow, 1, 1, 1);

   if (messageLabel)
      annotationLayout->addWidget(messageLabel, labelRow, 2, 1, 1);

   annotationLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding), totalAnnot, 4);

   mAnotation = new QFrame();
   mAnotation->setObjectName("AnnotationFrame");
   mAnotation->setLayout(annotationLayout);

   mScrollArea->setWidget(mAnotation);
   mScrollArea->setWidgetResizable(true);
}

QLabel *FileBlameWidget::createDateLabel(const Annotation &annotation, bool isFirst)
{
   auto isWip = annotation.sha == ZERO_SHA;
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

QLabel *FileBlameWidget::createAuthorLabel(const Annotation &annotation, bool isFirst)
{
   const auto authorLabel = new QLabel(annotation.author);
   authorLabel->setObjectName(isFirst ? QString("authorPrimusInterPares") : QString("authorFirstOfItsName"));
   authorLabel->setFont(mInfoFont);
   authorLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

   return authorLabel;
}

ClickableFrame *FileBlameWidget::createMessageLabel(const Annotation &annotation, bool isFirst)
{
   auto isWip = annotation.sha == ZERO_SHA.left(8);
   const auto revision = mGit->revLookup(annotation.sha);
   const auto commitMsg = revision ? revision->shortLog() : QString("WIP");

   const auto messageLabel = new ClickableFrame(commitMsg, Qt::AlignTop | Qt::AlignLeft);
   messageLabel->setObjectName(isFirst ? QString("primusInterPares") : QString("firstOfItsName"));
   messageLabel->setToolTip(
       QString("<p>%1</p><p>%2</p>").arg(annotation.sha, revision ? revision->shortLog() : QString("Local changes")));
   messageLabel->setFont(mInfoFont);

   if (!isWip)
   {
      const auto dtSinceEpoch = annotation.dateTime.toSecsSinceEpoch();
      const auto colorIndex = qCeil((kSecondsNewest - dtSinceEpoch) / kIncrementSecs);
      messageLabel->setStyleSheet(QString("QLabel { border-right: 5px solid rgb(%1) }").arg(kBorderColors[colorIndex]));
   }
   else
      messageLabel->setStyleSheet("QLabel { border-right: 5px solid #D89000 }");

   connect(messageLabel, &ClickableFrame::clicked, this,
           [this, annotation]() { emit signalCommitSelected(annotation.sha); });

   return messageLabel;
}

QLabel *FileBlameWidget::createNumLabel(const Annotation &annotation, int row)
{
   auto isWip = annotation.sha == ZERO_SHA.left(8);
   const auto numberLabel = new QLabel(QString::number(row + 1));
   numberLabel->setFont(mCodeFont);
   numberLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
   numberLabel->setObjectName("numberLabel");
   numberLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

   if (!isWip)
   {
      const auto dtSinceEpoch = annotation.dateTime.toSecsSinceEpoch();
      const auto colorIndex = qCeil((kSecondsNewest - dtSinceEpoch) / kIncrementSecs);
      numberLabel->setStyleSheet(QString("QLabel { border-left: 5px solid rgb(%1) }").arg(kBorderColors[colorIndex]));
   }
   else
      numberLabel->setStyleSheet("QLabel { border-left: 5px solid #D89000 }");

   return numberLabel;
}

QLabel *FileBlameWidget::createCodeLabel(const Annotation &annotation)
{
   const auto contentLabel = new QLabel(annotation.content);
   contentLabel->setFont(mCodeFont);
   contentLabel->setObjectName("normalLabel");

   return contentLabel;
}
