#include "References.h"

#include <ReferenceType.h>

void References::configure(const QString &refName, bool isCurrentBranch, const QString &prevRefSha)
{
   if (refName.startsWith("refs/tags/"))
   {
      if (refName.endsWith("^{}"))
      {
         // we assume that a tag dereference follows strictly
         // the corresponding tag object in rLst. So the
         // last added tag is a tag object, not a commit object
         tags.append(refName.mid(10, refName.length() - 13));

         // store tag object. Will be used to fetching
         // tag message (if any) when necessary.
         tagObj = prevRefSha;
      }
      else
         tags.append(refName.mid(10));

      type |= TAG;
   }
   else if (refName.startsWith("refs/heads/"))
   {
      branches.append(refName.mid(11));
      type |= BRANCH;

      if (isCurrentBranch)
         type |= CUR_BRANCH;
   }
   else if (refName.startsWith("refs/remotes/") && !refName.endsWith("HEAD"))
   {
      remoteBranches.append(refName.mid(13));
      type |= RMT_BRANCH;
   }
   else if (!refName.startsWith("refs/bases/") && !refName.endsWith("HEAD"))
   {
      refs.append(refName);
      type |= REF;
   }
}
