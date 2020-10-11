#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2020  Francesc Martinez
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

#include <Lane.h>
#include <References.h>

class CommitInfo
{
public:
   enum class Field
   {
      SHA,
      PARENTS_SHA,
      COMMITER,
      AUTHOR,
      DATE,
      SHORT_LOG,
      LONG_LOG
   };

   CommitInfo() = default;
   explicit CommitInfo(const QString sha, const QStringList &parents, const QChar &boundary, const QString &commiter,
                       const QDateTime &commitDate, const QString &author, const QString &log,
                       const QString &longLog = QString(), bool isSigned = false, const QString &gpgKey = QString());
   bool operator==(const CommitInfo &commit) const;
   bool operator!=(const CommitInfo &commit) const;

   QString getFieldStr(CommitInfo::Field field) const;

   void setBoundary(QChar info) { mBoundaryInfo = std::move(info); }
   bool isBoundary() const { return mBoundaryInfo == '-'; }
   int parentsCount() const;
   QString parent(int idx) const;
   QStringList parents() const;

   QString sha() const { return mSha; }
   QString committer() const { return mCommitter; }
   QString author() const { return mAuthor; }
   QString authorDate() const { return QString::number(mCommitDate.toSecsSinceEpoch()); }
   QString shortLog() const { return mShortLog; }
   QString longLog() const { return mLongLog; }
   QString fullLog() const { return QString("%1\n\n%2").arg(mShortLog, mLongLog.trimmed()); }

   bool isValid() const;
   bool isWip() const { return mSha == ZERO_SHA; }

   void setLanes(const QVector<Lane> &lanes) { mLanes = lanes; }
   QVector<Lane> getLanes() const { return mLanes; }
   Lane getLane(int i) const { return mLanes.at(i); }
   int getLanesCount() const { return mLanes.count(); }
   int getActiveLane() const;

   void addReference(References::Type type, const QString &reference);
   void addReferences(const References &refs) { mReferences = refs; }
   QStringList getReferences(References::Type type) const { return mReferences.getReferences(type); }
   bool hasReferences() const { return !mReferences.isEmpty(); }

   void addChildReference(CommitInfo *commit) { mChilds.insert(commit->sha(), commit); }
   QList<CommitInfo *> getChilds() const { return mChilds.values(); }
   bool hasChilds() const { return !mChilds.empty(); }

   void clearReferences() { mReferences.clear(); }

   bool isSigned() const { return mSigned; }
   QString getGpgKey() const { return mGpgKey; }

   static const QString ZERO_SHA;
   static const QString INIT_SHA;

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
   QVector<Lane> mLanes;
   References mReferences;
   QMap<QString, CommitInfo *> mChilds;
   bool mSigned = false;
   QString mGpgKey;
};
