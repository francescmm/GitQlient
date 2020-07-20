#include <CommitInfoPanel.h>
#include <CommitInfo.h>

#include <QVBoxLayout>
#include <QLabel>

CommitInfoPanel::CommitInfoPanel(QWidget *parent)
   : QFrame(parent)
   , mLabelSha(new QLabel())
   , mLabelTitle(new QLabel())
   , mLabelDescription(new QLabel())
   , mLabelAuthor(new QLabel())
   , mLabelDateTime(new QLabel())
{
   mLabelSha->setObjectName("labelSha");
   mLabelSha->setAlignment(Qt::AlignCenter);
   mLabelSha->setWordWrap(true);

   QFont font1;
   font1.setBold(true);
   font1.setWeight(QFont::Bold);
   mLabelTitle->setFont(font1);
   mLabelTitle->setAlignment(Qt::AlignCenter);
   mLabelTitle->setWordWrap(true);
   mLabelTitle->setObjectName("labelTitle");

   mLabelDescription->setWordWrap(true);
   mLabelDescription->setObjectName("labelDescription");

   mLabelAuthor->setObjectName("labelAuthor");

   mLabelDateTime->setObjectName("labelDateTime");

   const auto descriptionLayout = new QVBoxLayout(this);
   descriptionLayout->setContentsMargins(0, 0, 0, 0);
   descriptionLayout->setSpacing(0);
   descriptionLayout->addWidget(mLabelSha);
   descriptionLayout->addWidget(mLabelTitle);
   descriptionLayout->addWidget(mLabelDescription);
   descriptionLayout->addWidget(mLabelAuthor);
   descriptionLayout->addWidget(mLabelDateTime);
}

void CommitInfoPanel::configure(const CommitInfo &commit)
{
   mLabelSha->setText(commit.sha());

   const auto authorName = commit.committer().split("<").first();
   mLabelTitle->setText(commit.shortLog());
   mLabelAuthor->setText(authorName);

   QDateTime commitDate = QDateTime::fromSecsSinceEpoch(commit.authorDate().toInt());
   mLabelDateTime->setText(commitDate.toString("dd/MM/yyyy hh:mm"));

   const auto description = commit.longLog();
   mLabelDescription->setText(description.isEmpty() ? "<No description provided>" : description);

   auto f = mLabelDescription->font();
   f.setItalic(description.isEmpty());
   mLabelDescription->setFont(f);
}

void CommitInfoPanel::clear()
{
   mLabelSha->clear();
   mLabelTitle->clear();
   mLabelAuthor->clear();
   mLabelDateTime->clear();
   mLabelDescription->clear();
}
