#include "Terminal.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QApplication>
#include <QFile>

#include "common.h"
#include "git.h"

using namespace QGit;

QTextBrowser *QGit::kErrorLogBrowser = nullptr;

Terminal::Terminal(QSharedPointer<Git> git)
   : QDialog()
   , mGit(git)
   , leGitCommand(new QLineEdit())
   , outputTerminal(new QTextBrowser())
{
   setWindowTitle(tr("GitQlient terminal"));
   setMinimumSize(800, 400);
   setAttribute(Qt::WA_DeleteOnClose);

   QFile styles(":/stylesheet.css");

   if (styles.open(QIODevice::ReadOnly))
   {
      setStyleSheet(QString::fromUtf8(styles.readAll()));
      styles.close();
   }

   QGit::kErrorLogBrowser = outputTerminal;

   leGitCommand->setObjectName("leGitCommand");
   leGitCommand->setPlaceholderText(tr("Enter Git command..."));

   outputTerminal->setObjectName("outputTerminal");

   const auto vLayout = new QVBoxLayout(this);
   vLayout->setSpacing(0);
   vLayout->setContentsMargins(QMargins());
   vLayout->addWidget(leGitCommand);
   vLayout->addWidget(outputTerminal);

   connect(leGitCommand, &QLineEdit::returnPressed, this, &Terminal::executeCommand);
   connect(this, &Terminal::destroyed, []() { QGit::kErrorLogBrowser = nullptr; });
}

void Terminal::executeCommand()
{
   if (!leGitCommand->text().isEmpty())
   {
      const auto order = leGitCommand->text();
      if (order == "clear")
         outputTerminal->clear();
      else if (order == "update")
         emit signalUpdateUi();
      else if (order == "exit" || order == "quit")
         close();
      else
      {
         QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
         const auto ret = mGit->run(leGitCommand->text());
         QApplication::restoreOverrideCursor();
         outputTerminal->setText(ret.second);
      }
   }

   leGitCommand->clear();
}

int Terminal::exec()
{
   return QDialog::exec();
}
