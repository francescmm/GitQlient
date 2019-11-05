#include "ShaFilterProxyModel.h"

#include <RepositoryModelColumns.h>

ShaFilterProxyModel::ShaFilterProxyModel(QObject *parent)
   : QSortFilterProxyModel(parent)
{
}
bool ShaFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{

   const auto shaIndex = sourceModel()->index(sourceRow, static_cast<int>(RepositoryModelColumns::SHA), sourceParent);
   const auto sha = sourceModel()->data(shaIndex).toString();
   return mAcceptedShas.contains(sha);
}
