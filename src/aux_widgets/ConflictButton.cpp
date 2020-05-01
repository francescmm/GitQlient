#include "ConflictButton.h"

#include <GitBase.h>
#include <GitLocal.h>
#include <GitQlientSettings.h>
#include <QLogger.h>

#include <QPushButton>
#include <QHBoxLayout>
#include <QProcess>

using namespace QLogger;

ConflictButton::ConflictButton(const QString &filename, bool inConflict, const QSharedPointer<GitBase> &git,
                               QWidget *parent)
   : QFrame(parent)
   , mGit(git)
   , mFileName(filename)
   , mFile(new QPushButton(mFileName))
   , mEdit(new QPushButton())
   , mResolve(new QPushButton())
   , mUpdate(new QPushButton())
{
   mFile->setCheckable(true);
   mFile->setChecked(inConflict);

   mEdit->setIcon(QIcon(":/icons/text-file"));
   mEdit->setFixedSize(30, 30);
   mResolve->setIcon(QIcon(":/icons/check"));
   mResolve->setFixedSize(30, 30);
   mUpdate->setIcon(QIcon(":/icons/refresh"));
   mUpdate->setFixedSize(30, 30);

   const auto layout = new QHBoxLayout(this);
   layout->setSpacing(0);
   layout->setContentsMargins(QMargins());
   layout->addWidget(mFile);
   layout->addWidget(mEdit);
   layout->addWidget(mUpdate);
   layout->addWidget(mResolve);

   mUpdate->setVisible(inConflict);
   mResolve->setVisible(inConflict);

   connect(mFile, &QPushButton::toggled, this, &ConflictButton::toggled);
   connect(mEdit, &QPushButton::clicked, this, &ConflictButton::openFileEditor);
   connect(mResolve, &QPushButton::clicked, this, &ConflictButton::resolveConflict);
   connect(mUpdate, &QPushButton::clicked, this, [this]() { emit updateRequested(); });
}

void ConflictButton::setChecked(bool checked)
{
   mFile->setChecked(checked);
}

void ConflictButton::setInConflict(bool inConflict)
{
   mUpdate->setVisible(inConflict);
   mResolve->setVisible(inConflict);
}

void ConflictButton::resolveConflict()
{
   QScopedPointer<GitLocal> git(new GitLocal(mGit));
   const auto ret = git->markFileAsResolved(mFileName);

   if (ret.success)
   {
      setInConflict(false);
      emit resolved();
   }
}

void ConflictButton::openFileEditor()
{
   const auto fullPath = QString(mGit->getWorkingDir() + "/" + mFileName);

   // get external diff viewer command
   GitQlientSettings settings;
   const auto editor
       = settings.value(GitQlientSettings::ExternalEditorKey, GitQlientSettings::ExternalEditorValue).toString();

   auto processCmd = editor;

   if (!editor.contains("%1"))
      processCmd.append(" %1");

   processCmd = processCmd.arg(fullPath);

   const auto externalEditor = new QProcess();
   connect(externalEditor, SIGNAL(finished(int, QProcess::ExitStatus)), externalEditor, SLOT(deleteLater()));

   externalEditor->setWorkingDirectory(mGit->getWorkingDir());

   externalEditor->start(processCmd);

   if (!externalEditor->waitForStarted(10000))
   {
      QString text = QString("Cannot start external editor: %1.").arg(editor);

      QLog_Error("UI", text);

      delete externalEditor;
   }
}
