#pragma once

#include <QMap>
#include <QString>

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

const QMap<StateType, QString> kStateTypeMap {
   { StateType::Empty, "Empty" },
   { StateType::Active, "Active" },
   { StateType::Inactive, "Inactive" },
   { StateType::MergeFork, "MergeFork" },
   { StateType::MergeForkRight, "MergeForkRight" },
   { StateType::MergeForkLeft, "MergeForkLeft" },
   { StateType::Join, "Join" },
   { StateType::JoinRight, "JoinRight" },
   { StateType::JoinLeft, "JoinLeft" },
   { StateType::Head, "Head" },
   { StateType::HeadRight, "HeadRight" },
   { StateType::HeadLeft, "HeadLeft" },
   { StateType::Tail, "Tail" },
   { StateType::TailRight, "TailRight" },
   { StateType::TailLeft, "TailLeft" },
   { StateType::Cross, "Cross" },
   { StateType::CrossEmpty, "CrossEmpty" },
   { StateType::Initial, "Initial" },
   { StateType::Branch, "Branch" }
};
}
