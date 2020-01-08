#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This program is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 2 of the License, or (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <QVector>
#include <QStringList>
#include <QDateTime>

enum class LaneType;

class CommitInfo
{
public:
   CommitInfo() = default;
   CommitInfo(const QString &sha, const QStringList &parents, const QString &author, long long secsSinceEpoch,
              const QString &log, const QString &longLog, int idx);
   CommitInfo(const QByteArray &b, int idx);
   bool operator==(const CommitInfo &commit) const;
   bool operator!=(const CommitInfo &commit) const;
   bool isBoundary() const { return mBoundaryInfo == '-'; }
   int parentsCount() const { return mParentsSha.count(); }
   QString parent(int idx) const { return mParentsSha.count() > idx ? mParentsSha.at(idx) : QString(); }
   QStringList parents() const { return mParentsSha; }
   QString sha() const { return mSha; }
   QString committer() const { return mCommitter; }
   QString author() const { return mAuthor; }
   QString authorDate() const { return QString::number(mCommitDate.toSecsSinceEpoch()); }
   QString shortLog() const { return mShortLog; }
   QString longLog() const { return mLongLog; }
   QString fullLog() const { return QString("%1\n\n%2").arg(mShortLog, mLongLog.trimmed()); }
   bool isValid() const;

   QVector<LaneType> lanes;
   int orderIdx = -1;
   bool isDiffCache = false;

   static const QString ZERO_SHA;

private:
   QChar mBoundaryInfo;
   QString mSha;
   QStringList mParentsSha;
   QString mCommitter;
   QString mAuthor;
   QDateTime mCommitDate;
   QString mShortLog;
   QString mLongLog;
   QString mDiff;
};
