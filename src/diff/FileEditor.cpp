#include "FileEditor.h"

#include <FileDiffEditor.h>
#include <GitQlientStyles.h>
#include <Highlighter.h>

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>

FileEditor::FileEditor(QWidget *parent)
   : QFrame(parent)
   , mFileEditor(new FileDiffEditor())
   , mSaveBtn(new QPushButton())
   , mCloseBtn(new QPushButton())
   , mFilePathLabel(new QLabel())
   , mHighlighter(new Highlighter(mFileEditor->document()))
{
   mSaveBtn->setIcon(QIcon(":/icons/save"));
   connect(mSaveBtn, &QPushButton::clicked, this, [this]() {
      const auto currentContent = mFileEditor->toPlainText();

      if (currentContent != mLoadedContent)
         saveTextInFile(currentContent);
   });

   mCloseBtn->setIcon(QIcon(":/icons/close"));
   connect(mCloseBtn, &QPushButton::clicked, this, &FileEditor::finishEdition);

   const auto optionsLayout = new QHBoxLayout();
   optionsLayout->setContentsMargins(QMargins());
   optionsLayout->setSpacing(10);
   optionsLayout->addWidget(mSaveBtn);
   optionsLayout->addStretch();
   optionsLayout->addWidget(mFilePathLabel);
   optionsLayout->addStretch();
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

   mFilePathLabel->setText(mFileName);

   QFile f(mFileName);

   if (f.open(QIODevice::ReadOnly))
   {
      mLoadedContent = f.readAll();
      f.close();
   }

   mFileEditor->loadDiff(mLoadedContent, {});

   isEditing = true;
}

void FileEditor::finishEdition()
{
   if (isEditing)
   {
      const auto currentContent = mFileEditor->toPlainText();
      QFile f(mFileName);
      QString fileContent;

      if (f.open(QIODevice::ReadOnly))
      {
         fileContent = f.readAll();
         f.close();
      }

      if (currentContent != fileContent)
      {
         const auto alert = new QMessageBox(QMessageBox::Question, tr("Unsaved changes"),
                                            tr("The current text was modified. Do you want to save the changes?"));
         alert->setStyleSheet(GitQlientStyles::getInstance()->getStyles());
         alert->addButton("Discard", QMessageBox::ButtonRole::RejectRole);
         alert->addButton("Save", QMessageBox::ButtonRole::AcceptRole);

         if (alert->exec() == QMessageBox::Accepted)
            saveTextInFile(currentContent);
      }

      isEditing = false;

      emit signalEditionClosed();
   }
}

void FileEditor::saveFile()
{
   const auto currentContent = mFileEditor->toPlainText();

   saveTextInFile(currentContent);
}

void FileEditor::saveTextInFile(const QString &content) const
{
   QFile f(mFileName);

   if (f.open(QIODevice::WriteOnly))
   {
      f.write(content.toUtf8());
      f.close();
   }
}
