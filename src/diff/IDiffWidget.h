#pragma once

#include <QFrame>

class GitBase;
class RevisionsCache;

class IDiffWidget : public QFrame
{
   Q_OBJECT
signals:

public:
   explicit IDiffWidget(const QSharedPointer<GitBase> &git, QSharedPointer<RevisionsCache> cache,
                        QWidget *parent = nullptr);

   /*!
    \brief Reloads the current diff in case the user loaded the work in progress as base commit.

   */
   virtual bool reload() = 0;

   /*!
    \brief Gets the current SHA.

   \return QString The current SHA.
       */
   QString getCurrentSha() const { return mCurrentSha; }
   /*!
    \brief Gets the SHA agains the diff is comparing to.

   \return QString The SHA that the diff is compared to.
       */
   QString getPreviousSha() const { return mPreviousSha; }

protected:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<RevisionsCache> mCache;
   QString mCurrentSha;
   QString mPreviousSha;
};
