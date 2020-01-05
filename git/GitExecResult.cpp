#include "GitExecResult.h"

GitExecResult::GitExecResult(const QPair<bool, QVariant> &result)
   : success(result.first)
   , output(result.second)
{
}

GitExecResult::GitExecResult(const QPair<bool, QString> &result)
   : success(result.first)
   , output(result.second)
{
}

GitExecResult &GitExecResult::operator=(const QPair<bool, QString> &result)
{
   success = result.first;
   output = result.second;

   return *this;
}
