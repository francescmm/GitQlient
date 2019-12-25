#pragma once

enum RefType
{
   TAG = 1,
   BRANCH = 2,
   RMT_BRANCH = 4,
   CUR_BRANCH = 8,
   REF = 16,
   APPLIED = 32,
   UN_APPLIED = 64,
   ANY_REF = 127
};
