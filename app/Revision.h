#pragma once

#include <QVector>

enum class LaneType;

class Revision
{
public:
   Revision() = default;
   Revision(const QByteArray &b, uint s, int idx, int *next);
   bool isBoundary() const;
   uint parentsCount() const;
   QString parent(int idx) const;
   QStringList parents() const;
   QString sha() const;
   QString committer() const;
   QString author() const;
   QString authorDate() const;
   QString shortLog() const;
   QString longLog() const;
   QString diff() const;

   QVector<LaneType> lanes;
   QVector<int> children;
   QVector<int> descRefs; // list of descendant refs index, normally tags
   QVector<int> ancRefs; // list of ancestor refs index, normally tags
   QVector<int> descBranches; // list of descendant branches index
   int descRefsMaster; // in case of many Revision have the same descRefs, ancRefs or
   int ancRefsMaster; // descBranches these are stored only once in a Revision pointed
   int descBrnMaster; // by corresponding index xxxMaster
   int orderIdx = -1;

private:
   void setup() const;
   int indexData(bool quick, bool withDiff) const;
   QString mid(int start, int len) const;
   QString midSha(int start, int len) const;

   QByteArray ba;
   int start;
   mutable int parentsCnt, shaStart, comStart, autStart, autDateStart;
   mutable int sLogStart, sLogLen, lLogStart, lLogLen, diffStart, diffLen;
   mutable bool indexed;

public:
   bool isDiffCache, isApplied, isUnApplied; // put here to optimize padding
};
