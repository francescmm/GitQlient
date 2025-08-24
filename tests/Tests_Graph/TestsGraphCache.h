#pragma once

#include <QCoreApplication>

class TestsGraphCache : public QObject
{
   Q_OBJECT

private slots:
   void initTestCase();
   void cleanupTestCase();
   void test_buildTemporalLoom();
};
