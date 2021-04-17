#pragma once

#include <QDialog>

namespace Ui
{
class SquashDlg;
}

class GitCache;
class CheckBox;

class SquashDlg : public QDialog
{
   Q_OBJECT

public:
   explicit SquashDlg(QSharedPointer<GitCache> cache, const QStringList &shas, QWidget *parent = nullptr);
   ~SquashDlg();

private:
   struct CommitState
   {
      CheckBox *checkbox;
      QString sha;
   };

   QSharedPointer<GitCache> mCache;
   Ui::SquashDlg *ui;
   QVector<CommitState> mCheckboxList;
   int mTitleMaxLength = 50;

   void updateCounter(const QString &text);
};
