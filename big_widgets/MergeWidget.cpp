#include "MergeWidget.h"

#include <FileDiffWidget.h>

#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>

MergeWidget::MergeWidget(const QSharedPointer<Git> git, QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mCenterStackedWidget(new QStackedWidget())
   , mCommitTitle(new QLineEdit())
   , mDescription(new QTextEdit())
   , mMergeBtn(new QPushButton(tr("Merge & Commit")))
   , mAbortBtn(new QPushButton(tr("Abort merge")))
{
   mCenterStackedWidget->setCurrentIndex(0);
   mCenterStackedWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   const auto conflictBtnFrame = new QFrame();
   conflictBtnFrame->setObjectName("DiffButtonsFrame");

   mConflictBtnContainer = new QVBoxLayout(conflictBtnFrame);
   mConflictBtnContainer->setContentsMargins(QMargins());
   mConflictBtnContainer->setSpacing(5);
   mConflictBtnContainer->setAlignment(Qt::AlignTop);

   const auto conflictScrollArea = new QScrollArea();
   conflictScrollArea->setWidget(conflictBtnFrame);
   conflictScrollArea->setWidgetResizable(true);
   conflictScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

   const auto autoMergedBtnFrame = new QFrame();
   autoMergedBtnFrame->setObjectName("DiffButtonsFrame");

   mAutoMergedBtnContainer = new QVBoxLayout(autoMergedBtnFrame);
   mAutoMergedBtnContainer->setContentsMargins(QMargins());
   mAutoMergedBtnContainer->setSpacing(5);
   mAutoMergedBtnContainer->setAlignment(Qt::AlignTop);

   const auto autoMergedScrollArea = new QScrollArea();
   autoMergedScrollArea->setWidget(autoMergedBtnFrame);
   autoMergedScrollArea->setWidgetResizable(true);
   autoMergedScrollArea->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

   mCommitTitle->setObjectName(QString::fromUtf8("leCommitTitle"));
   mDescription->setMaximumHeight(125);
   mDescription->setObjectName(QString::fromUtf8("teDescription"));
   mDescription->setLineWrapMode(QTextEdit::WidgetWidth);
   mDescription->setReadOnly(false);
   mDescription->setAcceptRichText(false);

   const auto mergeBtnLayout = new QHBoxLayout();
   mergeBtnLayout->setContentsMargins(QMargins());
   mergeBtnLayout->addWidget(mAbortBtn);
   mergeBtnLayout->addStretch();
   mergeBtnLayout->addWidget(mMergeBtn);

   const auto mergeInfoWidget = new QFrame();
   const auto mergeInfoLayout = new QVBoxLayout(mergeInfoWidget);
   mergeInfoLayout->setContentsMargins(QMargins());
   mergeInfoLayout->setSpacing(0);
   mergeInfoLayout->addWidget(mCommitTitle);
   mergeInfoLayout->addWidget(mDescription);
   mergeInfoLayout->addSpacerItem(new QSpacerItem(1, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
   mergeInfoLayout->addLayout(mergeBtnLayout);

   const auto mergeLayout = new QVBoxLayout();
   mergeLayout->setContentsMargins(QMargins());
   mergeLayout->setSpacing(10);
   mergeLayout->addWidget(conflictScrollArea);
   mergeLayout->addWidget(autoMergedScrollArea);
   mergeLayout->addWidget(mergeInfoWidget);

   const auto layout = new QHBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->addLayout(mergeInfoLayout);
   layout->addWidget(mCenterStackedWidget);

   connect(mAbortBtn, &QPushButton::clicked, this, []() {});
   connect(mMergeBtn, &QPushButton::clicked, this, []() {});
}
