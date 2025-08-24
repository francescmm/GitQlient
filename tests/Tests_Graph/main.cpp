#include <TestsGraphCache.h>

#include <QTest>

int main(int argc, char *argv[])
{
   int status = 0;
   {
      TestsGraphCache tc;
      status |= QTest::qExec(&tc, argc, argv);
   }
   return status;
}
