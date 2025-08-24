#include <Commit.h>
#include <GraphCache.h>

namespace Graph
{
Cache::Cache(QObject *parent)
   : QObject(parent)
{
}

void Cache::init()
{
   mTemporalLoom = {};
}

void Cache::createMultiverse(std::span<Commit> commits)
{
   for (auto &commit : commits)
   {
      auto univers = mTemporalLoom.createTimeline(commit.sha, commit.parents());
      mMultiverse[commit.sha] = univers;
   }
}

void Cache::addTimeline(const Commit &commit)
{
   Timeline u;
   u.setType(0, StateType::Active);
   mMultiverse[commit.sha] = std::move(u);
}

int Cache::timelinesCount(const QString &sha) const
{
   return mMultiverse.value(sha).count();
}

State Cache::getTimelineAt(const QString &sha, int index) const
{
   if (const auto &timeline = mMultiverse.value(sha);
       index >= 0 && index < timeline.count())
   {
      return timeline.at(index);
   }

   return State();
}

int Cache::getSacredTimeline(const QString &sha) const
{
   const auto &timeline = mMultiverse.value(sha);
   for (int i = 0; i < timeline.count(); ++i)
   {
      const auto &state = timeline.at(i);
      if (state.isActive())
         return i;
   }
   return -1;
}

}
