#pragma once

#include <TemporalLoom.h>

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

#include <span>

class Commit;

namespace Graph
{
class Cache : public QObject
{
   Q_OBJECT

public:
   explicit Cache(QObject *parent = nullptr);

   void init();
   void createMultiverse(std::span<Commit> commits);
   void addTimeline(const Commit &commit);

   int timelinesCount(const QString &sha) const;
   State getTimelineAt(const QString &sha, int index) const;
   int getSacredTimeline(const QString &sha) const;

private:
   TemporalLoom mTemporalLoom;
   QHash<QString, Timeline> mMultiverse;
};
}
