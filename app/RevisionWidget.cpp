#include <RevisionWidget.h>

#include <Revision.h>
#include <FileListWidget.h>
#include <git.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QDateTime>

#include <QLogger.h>

using namespace QLogger;

RevisionWidget::RevisionWidget(QSharedPointer<Git> git, QWidget *parent)
   : QWidget(parent)
   , mGit(git)
   , labelSha(new QLabel())
   , labelTitle(new QLabel())
   , labelDescription(new QLabel())
   , labelAuthor(new QLabel())
   , labelDateTime(new QLabel())
   , labelEmail(new QLabel())
   , fileListWidget(new FileListWidget(mGit))
   , labelModCount(new QLabel())
{
   labelSha->setObjectName("labelSha");
   labelSha->setAlignment(Qt::AlignCenter);
   labelSha->setWordWrap(true);

   QFont font1;
   font1.setBold(true);
   font1.setWeight(75);
   labelTitle->setFont(font1);
   labelTitle->setAlignment(Qt::AlignCenter);
   labelTitle->setWordWrap(true);
   labelTitle->setObjectName("labelTitle");

   labelDescription->setWordWrap(true);
   labelDescription->setObjectName("labelDescription");

   const auto commitInfoFrame = new QFrame();
   commitInfoFrame->setObjectName("commitInfoFrame");

   const auto verticalLayout_2 = new QVBoxLayout(commitInfoFrame);
   verticalLayout_2->setSpacing(15);
   verticalLayout_2->setContentsMargins(0, 0, 0, 0);
   verticalLayout_2->addWidget(labelAuthor);
   verticalLayout_2->addWidget(labelDateTime);
   verticalLayout_2->addWidget(labelEmail);

   const auto labelIcon = new QLabel();
   labelIcon->setScaledContents(false);
   labelIcon->setPixmap(QIcon(":/icons/file").pixmap(15, 15));

   fileListWidget->setObjectName("fileListWidget");
   QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
   sizePolicy.setHorizontalStretch(0);
   sizePolicy.setVerticalStretch(0);
   sizePolicy.setHeightForWidth(fileListWidget->sizePolicy().hasHeightForWidth());
   fileListWidget->setSizePolicy(sizePolicy);

   const auto gridLayout = new QGridLayout();
   gridLayout->setHorizontalSpacing(10);
   gridLayout->setVerticalSpacing(0);
   gridLayout->setContentsMargins(0, 0, 0, 0);
   gridLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 4, 1, 1);
   gridLayout->addWidget(new QLabel(tr("Files")), 0, 2, 1, 1);
   gridLayout->addWidget(labelIcon, 0, 1, 1, 1);
   gridLayout->addItem(new QSpacerItem(10, 30, QSizePolicy::Fixed, QSizePolicy::Minimum), 0, 0, 1, 1);
   gridLayout->addWidget(fileListWidget, 1, 0, 1, 5);
   gridLayout->addWidget(labelModCount, 0, 3, 1, 1);

   const auto verticalLayout = new QVBoxLayout(this);
   verticalLayout->setSpacing(10);
   verticalLayout->setContentsMargins(0, 0, 0, 0);
   verticalLayout->addWidget(labelSha);
   verticalLayout->addWidget(labelTitle);
   verticalLayout->addWidget(labelDescription);
   verticalLayout->addWidget(commitInfoFrame);
   verticalLayout->addLayout(gridLayout);

   connect(fileListWidget, &FileListWidget::itemDoubleClicked, this,
           [this](QListWidgetItem *item) { emit signalOpenFileCommit(mCurrentSha, mParentSha, item->text()); });
   connect(fileListWidget, &FileListWidget::signalShowFileHistory, this, &RevisionWidget::signalShowFileHistory);
   connect(fileListWidget, &FileListWidget::contextMenu, this, &RevisionWidget::signalOpenFileContextMenu);
}

void RevisionWidget::setup(Domain *rv)
{
   fileListWidget->setup(rv);
}

void RevisionWidget::setCurrentCommitSha(const QString &sha)
{
   clear();

   mCurrentSha = sha;
   mParentSha = sha;

   if (sha != ZERO_SHA and !sha.isEmpty())
   {
      const auto currentRev = const_cast<Revision *>(mGit->revLookup(sha));

      if (currentRev)
      {

         QLog_Info("UI", QString("Loading information of the commit {%1}").arg(sha));
         mCurrentSha = currentRev->sha();
         mParentSha = currentRev->parent(0);

         QDateTime commitDate = QDateTime::fromSecsSinceEpoch(currentRev->authorDate().toInt());
         labelSha->setText(sha);

         const auto author = currentRev->committer();
         const auto authorName = author.split("<").first();
         const auto email = author.split("<").last().split(">").first();

         labelEmail->setText(email);
         labelTitle->setText(currentRev->shortLog());
         labelAuthor->setText(authorName);
         labelDateTime->setText(commitDate.toString("dd/MM/yyyy hh:mm"));

         const auto description = currentRev->longLog().trimmed();
         labelDescription->setText(description.isEmpty() ? "No description provided." : description);

         auto f = labelDescription->font();
         f.setItalic(description.isEmpty());
         labelDescription->setFont(f);

         const auto files = mGit->getFiles(sha, currentRev->parent(0), true, "");
         fileListWidget->update(files, true);
         labelModCount->setText(QString("(%1)").arg(fileListWidget->count()));
      }
   }
}

QString RevisionWidget::getCurrentCommitSha() const
{
   return mCurrentSha;
}

void RevisionWidget::clear()
{
   fileListWidget->clear();
   labelSha->clear();
   labelEmail->clear();
   labelTitle->clear();
   labelAuthor->clear();
   labelDateTime->clear();
   labelDescription->clear();
}
