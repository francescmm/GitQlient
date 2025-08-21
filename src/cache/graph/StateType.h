#pragma once

namespace Graph
{
enum class StateType
{
   Empty,
   Active,
   Inactive,
   MergeFork,
   MergeForkRight,
   MergeForkLeft,
   Join,
   JoinRight,
   JoinLeft,
   Head,
   HeadRight,
   HeadLeft,
   Tail,
   TailRight,
   TailLeft,
   Cross,
   CrossEmpty,
   Initial,
   Branch
};
}
