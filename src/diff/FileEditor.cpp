#include "FileEditor.h"

#include <FileDiffEditor.h>
#include <GitQlientStyles.h>
#include <Highlighter.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

FileEditor::FileEditor(QWidget *parent)
   : QFrame(parent)
   , mFileEditor(new FileDiffEditor())
   , mCloseBtn(new QPushButton())
   , mHighlighter(new Highlighter(mFileEditor->document()))
{
   mCloseBtn->setIcon(QIcon(":/icons/close"));
   connect(mCloseBtn, &QPushButton::clicked, this, &FileEditor::finishEdition);

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(QMargins());
   optionsLayout->addStretch();
   optionsLayout->setSpacing(10);
   optionsLayout->addWidget(mCloseBtn);

   const auto layout = new QVBoxLayout(this);
   layout->setContentsMargins(QMargins());
   layout->setSpacing(10);
   layout->addLayout(optionsLayout);
   layout->addWidget(mFileEditor);
}

void FileEditor::editFile(const QString &fileName)
{
   mFileName = fileName;

   QFile f(mFileName);
   QString fileContent;

   if (f.open(QIODevice::ReadOnly))
   {
      mLoadedContent = f.readAll();
      f.close();
   }

   mFileEditor->loadDiff(mLoadedContent, {});
}

void FileEditor::finishEdition()
{
   const auto currentContent = mFileEditor->toPlainText();

   if (currentContent != mLoadedContent)
   {
      const auto alert = new QMessageBox(QMessageBox::Warning, tr("Unsaved changes"),
                                         tr("The current text was modified. Do you want to save the changes?"));
      alert->setStyleSheet(GitQlientStyles::getInstance()->getStyles());
      alert->addButton("Discard", QMessageBox::ButtonRole::RejectRole);
      alert->addButton("Save", QMessageBox::ButtonRole::AcceptRole);

      const auto ret = alert->exec();

      if (ret == QMessageBox::Accepted)
      {
         QFile f(mFileName);

         if (f.open(QIODevice::WriteOnly))
         {
            f.write(currentContent.toUtf8());
            f.close();
         }
      }
   }

   emit signalEditionClosed();
}
