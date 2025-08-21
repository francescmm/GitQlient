#pragma once

#include <QString>
#include <QVector>

class StateTracker
{
public:
   StateTracker() = default;
   void clear();
   int findNextSha(const QString &next, int pos) const;
   void setNextSha(int lane, const QString &sha);
   void append(const QString &sha);
   void removeLast();
   int count() const { return nextShaVec.count(); }
   const QString &at(int index) const { return nextShaVec.at(index); }

private:
   QVector<QString> nextShaVec;
};
