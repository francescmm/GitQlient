#include "Controls.h"

#include <git.h>

#include <QApplication>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QDir>
#include <QFileDialog>

Controls::Controls(QWidget *parent)
   : QWidget(parent)
   , mOpenRepo(new QToolButton())
   , mHome(new QToolButton())
   , mGoToBtn(new QToolButton())
   , mPullBtn(new QToolButton())
   , mPushBtn(new QToolButton())
   , mStashBtn(new QToolButton())
   , mPopBtn(new QToolButton())
   , mPruneBtn(new QToolButton())
   , mTerminalBtn(new QToolButton())
{
   mOpenRepo->setIcon(QIcon(":/icons/open_repo"));
   mOpenRepo->setIconSize(QSize(22, 22));
   mOpenRepo->setText("Open");
   mOpenRepo->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mHome->setIcon(QIcon(":/icons/home"));
   mHome->setIconSize(QSize(22, 22));
   mHome->setText("Home");
   mHome->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mGoToBtn->setIcon(QIcon(":/icons/go_to"));
   mGoToBtn->setIconSize(QSize(22, 22));
   mGoToBtn->setText(tr("Go to..."));
   mGoToBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   const auto menu = new QMenu(mPullBtn);
   QAction *action = nullptr;
   menu->addAction(action = new QAction("Fetch all"));
   connect(action, &QAction::triggered, this, &Controls::fetchAll);
   menu->addAction(action = new QAction("Pull"));
   connect(action, &QAction::triggered, this, &Controls::pullCurrentBranch);
   mPullBtn->setDefaultAction(action);
   menu->addSeparator();

   mPullBtn->setMenu(menu);
   mPullBtn->setIconSize(QSize(22, 22));
   mPullBtn->setText(tr("Pull"));
   mPullBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
   mPullBtn->setPopupMode(QToolButton::MenuButtonPopup);
   mPullBtn->setIcon(QIcon(":/icons/git_pull"));

   mPushBtn->setIcon(QIcon(":/icons/git_push"));
   mPushBtn->setIconSize(QSize(22, 22));
   mPushBtn->setText(tr("Push"));
   mPushBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mStashBtn->setIcon(QIcon(":/icons/git_stash"));
   mStashBtn->setIconSize(QSize(22, 22));
   mStashBtn->setText(tr("Stash"));
   mStashBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mPopBtn->setIcon(QIcon(":/icons/git_pop"));
   mPopBtn->setIconSize(QSize(22, 22));
   mPopBtn->setText(tr("Pop"));
   mPopBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mPruneBtn->setIcon(QIcon(":/icons/git_prune"));
   mPruneBtn->setIconSize(QSize(22, 22));
   mPruneBtn->setText(tr("Prune"));
   mPruneBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   mTerminalBtn->setIcon(QIcon(":/icons/terminal"));
   mTerminalBtn->setIconSize(QSize(22, 22));
   mTerminalBtn->setText(tr("Console"));
   mTerminalBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   const auto vLayout = new QHBoxLayout(this);
   vLayout->addStretch();
   vLayout->addWidget(mOpenRepo);
   vLayout->addWidget(mHome);
   vLayout->addWidget(mGoToBtn);
   vLayout->addWidget(mPullBtn);
   vLayout->addWidget(mPushBtn);
   vLayout->addWidget(mStashBtn);
   vLayout->addWidget(mPopBtn);
   vLayout->addWidget(mPruneBtn);
   vLayout->addWidget(mTerminalBtn);
   vLayout->addStretch();

   connect(mOpenRepo, &QToolButton::clicked, this, &Controls::openRepo);
   connect(mHome, &QToolButton::clicked, this, &Controls::signalGoBack);
   connect(mGoToBtn, &QToolButton::clicked, this, &Controls::openGoToDialog);
   connect(mPushBtn, &QToolButton::clicked, this, &Controls::pushCurrentBranch);
   connect(mStashBtn, &QToolButton::clicked, this, &Controls::stashCurrentWork);
   connect(mPopBtn, &QToolButton::clicked, this, &Controls::popStashedWork);
   connect(mPruneBtn, &QToolButton::clicked, this, &Controls::pruneBranches);
   connect(mTerminalBtn, &QToolButton::clicked, this, &Controls::showTerminal);
}

void Controls::openRepo()
{
   const QString dirName(QFileDialog::getExistingDirectory(this, "Choose the directory of a Git project"));

   if (!dirName.isEmpty())
   {
      QDir d(dirName);
      emit signalOpenRepo(d.absolutePath());
   }
}

void Controls::openGoToDialog()
{
   QLineEdit *goToSha = new QLineEdit();
   goToSha->setWindowModality(Qt::ApplicationModal);
   goToSha->setAttribute(Qt::WA_DeleteOnClose);

   connect(goToSha, &QLineEdit::returnPressed, this, [this, goToSha]() { emit signalGoToSha(goToSha->text()); });

   goToSha->show();
}

void Controls::pullCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   QString output;
   const auto ret = Git::getInstance()->pull(output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::fetchAll()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->fetch();
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::pushCurrentBranch()
{
   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->push();
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::stashCurrentWork()
{
   const auto ret = Git::getInstance()->stash();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::popStashedWork()
{
   const auto ret = Git::getInstance()->pop();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::pruneBranches()
{
   QByteArray output;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
   const auto ret = Git::getInstance()->prune(output);
   QApplication::restoreOverrideCursor();

   if (ret)
      emit signalRepositoryUpdated();
}

void Controls::showTerminal() {}
