#pragma once

#include <QFrame>

namespace DiffHelper
{
struct DiffChange;
}

class PrChangeListItem : public QFrame
{
public:
   explicit PrChangeListItem(DiffHelper::DiffChange change, QWidget *parent = nullptr);
};
