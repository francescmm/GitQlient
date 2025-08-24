#include <State.h>

#include <StateType.h>

namespace Graph
{
State::State(StateType type)
   : mType(type)
{
}

bool State::isHead() const
{
   switch (mType)
   {
      case StateType::Head:
      case StateType::HeadRight:
      case StateType::HeadLeft:
         return true;
      default:
         return false;
   }
}

bool State::isTail() const
{
   switch (mType)
   {
      case StateType::Tail:
      case StateType::TailRight:
      case StateType::TailLeft:
         return true;
      default:
         return false;
   }
}

bool State::isJoin() const
{
   switch (mType)
   {
      case StateType::Join:
      case StateType::JoinRight:
      case StateType::JoinLeft:
         return true;
      default:
         return false;
   }
}

bool State::isFreeLane() const
{
   switch (mType)
   {
      case StateType::Inactive:
      case StateType::Cross:
      case StateType::Join:
      case StateType::JoinRight:
      case StateType::JoinLeft:
         return true;
      default:
         return false;
   }
}

bool State::isMerge() const
{
   switch (mType)
   {
      case StateType::MergeFork:
      case StateType::MergeForkRight:
      case StateType::MergeForkLeft:
         return true;
      default:
         return false;
   }
}

bool State::isActive() const
{
   switch (mType)
   {
      case StateType::Active:
      case StateType::Initial:
      case StateType::Branch:
      case StateType::MergeFork:
      case StateType::MergeForkRight:
      case StateType::MergeForkLeft:
         return true;
      default:
         return false;
   }
}
}
