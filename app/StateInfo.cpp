#include "StateInfo.h"

StateInfo::StateInfo()
{
   clear();
}

StateInfo::StateInfo(const StateInfo &st)
{
   if (st.isLocked)
      nextS = st.curS;
   else
      curS = st.curS; // prevS is mot modified to allow a rollback
}

void StateInfo::S::clear()
{

   sha = fn = dtSha = "";
   isM = allM = false;
   sel = true;
}

bool StateInfo::S::operator==(const StateInfo::S &st) const
{

   if (&st == this)
      return true;

   return (sha == st.sha && fn == st.fn && dtSha == st.dtSha && sel == st.sel && isM == st.isM && allM == st.allM);
}

bool StateInfo::S::operator!=(const StateInfo::S &st) const
{

   return !(StateInfo::S::operator==(st));
}

void StateInfo::clear()
{

   nextS.clear();
   curS.clear();
   prevS.clear();
   isLocked = false;
}

StateInfo &StateInfo::operator=(const StateInfo &newState)
{

   if (&newState != this)
   {
      if (isLocked)
         nextS = newState.curS;
      else
         curS = newState.curS; // prevS is mot modified to allow a rollback
   }
   return *this;
}

bool StateInfo::operator==(const StateInfo &newState) const
{

   if (&newState == this)
      return true;

   return (curS == newState.curS); // compare is made on curS only
}

bool StateInfo::operator!=(const StateInfo &newState) const
{

   return !(StateInfo::operator==(newState));
}

bool StateInfo::isChanged(uint what) const
{

   bool ret = false;
   if (what & SHA)
      ret = (sha(true) != sha(false));

   if (!ret && (what & FILE_NAME))
      ret = (fileName(true) != fileName(false));

   if (!ret && (what & DIFF_TO_SHA))
      ret = (diffToSha(true) != diffToSha(false));

   if (!ret && (what & ALL_MERGE_FILES))
      ret = (allMergeFiles(true) != allMergeFiles(false));

   return ret;
}

void StateInfo::setLock(bool b)
{
   isLocked = b;

   if (b)
      nextS = curS;
}

void StateInfo::rollBack()
{
   if (nextS == curS)
      nextS.clear(); // invalidate to avoid infinite loop
   curS = prevS;
}

bool StateInfo::flushQueue()
{
   if (requestPending())
   {
      curS = nextS;
      return true;
   }
   return false;
}
