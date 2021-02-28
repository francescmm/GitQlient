#pragma once

#include <QObject>
#include <QSharedPointer>

#include <WipRevisionInfo.h>

class GitBase;
class GitCache;

class GitWip : public QObject
{
   Q_OBJECT
public:
   explicit GitWip(const QSharedPointer<GitBase> &git, const QSharedPointer<GitCache> &cache,
                   QObject *parent = nullptr);

   void updateUntrackedFiles() const;
   bool updateWip() const;
   WipRevisionInfo getWipInfo() const;

private:
   QSharedPointer<GitBase> mGit;
   QSharedPointer<GitCache> mCache;

   QVector<QString> getUntrackedFiles() const;
};
