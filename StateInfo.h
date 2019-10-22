#pragma once

#include <QString>

class StateInfo
{
public:
   enum Field
   {
      SHA = 1,
      FILE_NAME = 2,
      DIFF_TO_SHA = 4,
      ALL_MERGE_FILES = 8,
      ANY = 15
   };

   StateInfo();
   StateInfo(const StateInfo &st);
   StateInfo &operator=(const StateInfo &newState);
   bool operator==(const StateInfo &newState) const;
   bool operator!=(const StateInfo &newState) const;
   void clear();
   const QString sha(bool n = true) const { return n ? curS.sha : prevS.sha; }
   const QString fileName(bool n = true) const { return n ? curS.fn : prevS.fn; }
   const QString diffToSha(bool n = true) const { return n ? curS.dtSha : prevS.dtSha; }
   bool selectItem(bool n = true) const { return n ? curS.sel : prevS.sel; }
   bool isMerge(bool n = true) const { return n ? curS.isM : prevS.isM; }
   bool allMergeFiles(bool n = true) const { return n ? curS.allM : prevS.allM; }
   void setSha(const QString &s) { isLocked ? nextS.sha = s : curS.sha = s; }
   void setFileName(const QString &s) { isLocked ? nextS.fn = s : curS.fn = s; }
   void setDiffToSha(const QString &s) { isLocked ? nextS.dtSha = s : curS.dtSha = s; }
   void setSelectItem(bool b) { isLocked ? nextS.sel = b : curS.sel = b; }
   void setIsMerge(bool b) { isLocked ? nextS.isM = b : curS.isM = b; }
   void setAllMergeFiles(bool b) { isLocked ? nextS.allM = b : curS.allM = b; }
   bool isChanged(uint what = ANY) const;

private:
   friend class Domain;

   bool requestPending() const { return !nextS.sha.isEmpty() && nextS != curS; }
   void setLock(bool b);
   void commit() { prevS = curS; }
   void rollBack();
   bool flushQueue();

   class S
   {
   public:
      S() { clear(); }
      void clear();
      bool operator==(const S &newState) const;
      bool operator!=(const S &newState) const;

      QString sha;
      QString fn;
      QString dtSha;
      bool sel;
      bool isM;
      bool allM;
   };
   S curS; // current state, what returns from StateInfo::sha()
   S prevS; // previous good state, used to rollBack in case state update fails
   S nextS; // next queued state, waiting for current update to finish
   bool isLocked;
};
