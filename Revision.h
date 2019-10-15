#pragma once

#include <QVector>

class Revision
{
   // prevent implicit C++ compiler defaults
   Revision();
   Revision(const Revision &);
   Revision &operator=(const Revision &);

public:
   Revision(const QByteArray &b, uint s, int idx, int *next);
   bool isBoundary() const { return (ba.at(shaStart - 1) == '-'); }
   uint parentsCount() const { return parentsCnt; }
   QString parent(int idx) const;
   QStringList parents() const;
   QString sha() const { return QString::fromUtf8(ba.constData() + shaStart); }
   QString committer() const
   {
      setup();
      return mid(comStart, autStart - comStart - 1);
   }
   QString author() const
   {
      setup();
      return mid(autStart, autDateStart - autStart - 1);
   }
   QString authorDate() const
   {
      setup();
      return mid(autDateStart, 10);
   }
   QString shortLog() const
   {
      setup();
      return mid(sLogStart, sLogLen);
   }
   QString longLog() const
   {
      setup();
      return mid(lLogStart, lLogLen);
   }
   QString diff() const
   {
      setup();
      return mid(diffStart, diffLen);
   }

   QVector<int> lanes, children;
   QVector<int> descRefs; // list of descendant refs index, normally tags
   QVector<int> ancRefs; // list of ancestor refs index, normally tags
   QVector<int> descBranches; // list of descendant branches index
   int descRefsMaster; // in case of many Revision have the same descRefs, ancRefs or
   int ancRefsMaster; // descBranches these are stored only once in a Revision pointed
   int descBrnMaster; // by corresponding index xxxMaster
   int orderIdx;

private:
   inline void setup() const
   {
      if (!indexed)
         indexData(false, false);
   }
   int indexData(bool quick, bool withDiff) const;
   QString mid(int start, int len) const;
   QString midSha(int start, int len) const;

   QByteArray &ba; // reference here!
   int start;
   mutable int parentsCnt, shaStart, comStart, autStart, autDateStart;
   mutable int sLogStart, sLogLen, lLogStart, lLogLen, diffStart, diffLen;
   mutable bool indexed;

public:
   bool isDiffCache, isApplied, isUnApplied; // put here to optimize padding
};
