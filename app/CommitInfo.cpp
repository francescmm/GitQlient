#include "CommitInfo.h"

#include <QStringList>

CommitInfo::CommitInfo(const QString &sha, const QStringList &parents, const QString &author, long long secsSinceEpoch,
                       const QString &log, const QString &longLog, int idx)
{
   mSha = sha;
   mParentsSha = parents;
   mCommitter = author;
   mAuthor = author;
   mCommitDate = QDateTime::fromSecsSinceEpoch(secsSinceEpoch);
   mShortLog = log;
   mLongLog = longLog;
   orderIdx = idx;
}

CommitInfo::CommitInfo(const QByteArray &b, int idx)
   : orderIdx(idx)
{
   const auto fields = QString::fromUtf8(b).split('\n');

   if (fields.count() > 6)
   {
      auto combinedShas = fields.at(1);
      auto sha = combinedShas.split('X').first();
      mBoundaryInfo = sha.at(0);
      sha.remove(0, 1);
      mSha = sha;
      combinedShas = combinedShas.remove(0, mSha.size() + 1 + 1);
      mParentsSha = combinedShas.trimmed().split(' ', QString::SkipEmptyParts);
      mCommitter = fields.at(2);
      mAuthor = fields.at(3);
      mCommitDate = QDateTime::fromSecsSinceEpoch(fields.at(4).toInt());
      mShortLog = fields.at(5);

      for (auto i = 6; i < fields.count(); ++i)
         mLongLog += fields.at(i);
   }
}

bool CommitInfo::isValid() const
{
   QRegExp hexMatcher("^[0-9A-F]{40}$", Qt::CaseInsensitive);

   return !mSha.isEmpty() && hexMatcher.exactMatch(mSha);
}
