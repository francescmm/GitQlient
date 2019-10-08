#include "Controls.h"

#include "git.h"
#include "Terminal.h"

#include <QApplication>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QDir>
#include <QFileDialog>

Controls::Controls(QWidget *parent)
   : QFrame(parent)
   , mOpenRepo(new QToolButton())
   , mHome(new QToolButton())
   , mGoToBtn(new QToolButton())
   , mPullBtn(new QToolButton())
   , mPushBtn(new QToolButton())
   , mStashBtn(new QToolButton())
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

   auto action = menu->addAction(tr("Fetch all"));
   connect(action, &QAction::triggered, this, &Controls::fetchAll);

   action = menu->addAction(tr("Pull"));
   connect(action, &QAction::triggered, this, &Controls::pullCurrentBranch);
   mPullBtn->setDefaultAction(action);

   action = menu->addAction(tr("Prune"));
   connect(action, &QAction::triggered, this, &Controls::pruneBranches);
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

   const auto stashMenu = new QMenu(mStashBtn);

   action = stashMenu->addAction(tr("Push"));
   action->setIcon(QIcon(":/icons/git_stash"));
   connect(action, &QAction::triggered, this, &Controls::stashCurrentWork);

   // mStashBtn->setDefaultAction(action);
   action = stashMenu->addAction(tr("Pop"));
   action->setIcon(QIcon(":/icons/git_pop"));
   connect(action, &QAction::triggered, this, &Controls::popStashedWork);

   mStashBtn->setMenu(stashMenu);
   mStashBtn->setIcon(QIcon(":/icons/git_stash"));
   mStashBtn->setIconSize(QSize(22, 22));
   mStashBtn->setText(tr("Stash"));
   mStashBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
   mStashBtn->setPopupMode(QToolButton::MenuButtonPopup);

   mTerminalBtn->setIcon(QIcon(":/icons/terminal"));
   mTerminalBtn->setIconSize(QSize(22, 22));
   mTerminalBtn->setText(tr("Terminal"));
   mTerminalBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

   const auto vLayout = new QHBoxLayout(this);
   vLayout->setContentsMargins(10, 10, 10, 10);
   vLayout->addStretch();
   vLayout->addWidget(mOpenRepo);
   vLayout->addWidget(mHome);
   vLayout->addWidget(mGoToBtn);
   vLayout->addWidget(mPullBtn);
   vLayout->addWidget(mPushBtn);
   vLayout->addWidget(mStashBtn);
   vLayout->addWidget(mTerminalBtn);
   vLayout->addStretch();

   connect(mOpenRepo, &QToolButton::clicked, this, &Controls::openRepo);
   connect(mHome, &QToolButton::clicked, this, &Controls::signalGoBack);
   connect(mGoToBtn, &QToolButton::clicked, this, &Controls::openGoToDialog);
   connect(mPushBtn, &QToolButton::clicked, this, &Controls::pushCurrentBranch);
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
   const auto goToSha = new QLineEdit();
   goToSha->setWindowModality(Qt::ApplicationModal);
   goToSha->setAttribute(Qt::WA_DeleteOnClose);

   connect(goToSha, &QLineEdit::returnPressed, this, [this, goToSha]() { emit signalGoToSha(goToSha->text()); });

   goToSha->show();
}

void Controls::pullCurrentBranch()
{
   QString output;

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
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

void Controls::showTerminal()
{
   const auto terminal = new Terminal();
   connect(terminal, &Terminal::signalUpdateUi, this, &Controls::signalRepositoryUpdated);

   terminal->show();
}
