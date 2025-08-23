#include <TestsCore.h>

#include <QTest>

int main(int argc, char *argv[])
{
   int status = 0;
   {
      TestsCore tc;
      status |= QTest::qExec(&tc, argc, argv);
   }
   return status;
}
