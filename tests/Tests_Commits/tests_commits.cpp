#include <QCoreApplication>
#include <QtTest>

// add necessary includes here

class Tests_Commits : public QObject
{
   Q_OBJECT

public:
   Tests_Commits();
   ~Tests_Commits();

private slots:
   void initTestCase();
   void cleanupTestCase();
   void test_case1();
};

Tests_Commits::Tests_Commits()
{
}

Tests_Commits::~Tests_Commits()
{
}

void Tests_Commits::initTestCase()
{
}

void Tests_Commits::cleanupTestCase()
{
}

void Tests_Commits::test_case1()
{
}

QTEST_MAIN(Tests_Commits)

#include "tests_commits.moc"
