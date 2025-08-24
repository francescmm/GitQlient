#pragma once

#include <State.h>
#include <StateType.h>

#include <QVector>

namespace Graph
{
class Timeline
{
public:
   Timeline() = default;
   int findType(StateType type, int pos) const;
   void setType(int lane, StateType type);
   void append(StateType type);
   void removeLast();
   int count() const { return mStates.count(); }
   const State &at(int index) const { return mStates.at(index); }
   const State &last() const { return mStates.last(); }

private:
   QVector<State> mStates;
};
}
