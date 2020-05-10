#include "GitQlientTreeWidgetItem.h"

GitQlientTreeWidgetItem::GitQlientTreeWidgetItem(QTreeWidgetItem *parent)
   : QTreeWidgetItem(parent, UserType)
{
}

void GitQlientTreeWidgetItem::distancesToMaster(GitExecResult result)
{
   auto distanceToMaster = QString("Not found");
   const auto toMaster = result.output.toString();

   if (!toMaster.contains("fatal"))
   {
      distanceToMaster = toMaster;
      distanceToMaster.replace('\n', "");
      distanceToMaster.replace('\t', "\u2193 - ");
      distanceToMaster.append("\u2191");
   }

   setText(1, distanceToMaster);
}

void GitQlientTreeWidgetItem::distancesToOrigin(GitExecResult result)
{
   auto distanceToOrigin = QString("Local");
   const auto toOrigin = result.output.toString();

   if (!result.success && toOrigin.contains("Same branch", Qt::CaseInsensitive))
      distanceToOrigin = "-";
   else if (!toOrigin.contains("fatal"))
   {
      distanceToOrigin = toOrigin;
      distanceToOrigin.replace('\n', "");
      distanceToOrigin.replace('\t', "\u2193 - ");
      distanceToOrigin.append("\u2191");
   }

   setText(2, distanceToOrigin);
}
