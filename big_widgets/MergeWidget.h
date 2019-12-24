#pragma once

#include <QFrame>
#include <QMap>

class Git;
class QVBoxLayout;
class QPushButton;
class QStackedWidget;
class MergeInfoWidget;
class QLineEdit;
class QTextEdit;

class MergeWidget : public QFrame
{
   Q_OBJECT

signals:

public:
   explicit MergeWidget(const QSharedPointer<Git> git, QWidget *parent = nullptr);

private:
   QSharedPointer<Git> mGit;
   QVBoxLayout *mConflictBtnContainer = nullptr;
   QVBoxLayout *mAutoMergedBtnContainer = nullptr;
   QStackedWidget *mCenterStackedWidget = nullptr;
   QLineEdit *mCommitTitle = nullptr;
   QTextEdit *mDescription = nullptr;
   QPushButton *mMergeBtn = nullptr;
   QPushButton *mAbortBtn = nullptr;
   QMap<QString, QPushButton *> mConflictButtons;
};
