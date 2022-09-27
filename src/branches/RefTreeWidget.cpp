#include <RefTreeWidget.h>

#include <GitQlientBranchItemRole.h>

#include <QHeaderView>

using namespace GitQlient;

RefTreeWidget::RefTreeWidget(QWidget *parent)
   : QTreeWidget(parent)

{
   setContextMenuPolicy(Qt::CustomContextMenu);
   setAttribute(Qt::WA_DeleteOnClose);
   header()->setHidden(true);
}

int RefTreeWidget::focusOnBranch(const QString &itemText, int startSearchPos)
{
   const auto items = findChildItem(itemText);

   if (startSearchPos + 1 >= items.count())
      return -1;

   if (startSearchPos != -1)
   {
      auto itemToUnselect = items.at(startSearchPos);
      itemToUnselect->setSelected(false);
   }

   ++startSearchPos;

   auto itemToExpand = items.at(startSearchPos);

   if (itemToExpand->isSelected())
      return -1;

   itemToExpand->setExpanded(true);
   setCurrentItem(itemToExpand);
   setCurrentIndex(indexFromItem(itemToExpand));

   while (itemToExpand->parent())
   {
      itemToExpand->setExpanded(true);
      itemToExpand = itemToExpand->parent();
   }

   itemToExpand->setExpanded(true);

   return startSearchPos;
}

QVector<QTreeWidgetItem *> RefTreeWidget::findChildItem(const QString &text) const
{
   QModelIndexList indexes = model()->match(model()->index(0, 0, QModelIndex()), GitQlient::FullNameRole, text, -1,
                                            Qt::MatchContains | Qt::MatchRecursive);
   QVector<QTreeWidgetItem *> items;
   const int indexesSize = indexes.size();
   items.reserve(indexesSize);

   for (int i = 0; i < indexesSize; ++i)
      items.append(static_cast<QTreeWidgetItem *>(indexes.at(i).internalPointer()));

   return items;
}
