#include <TestsGraphCache.h>

#include <TemporalLoom.h>

#include <QTest>

void TestsGraphCache::initTestCase()
{
}

void TestsGraphCache::cleanupTestCase()
{
}

void TestsGraphCache::test_buildTemporalLoom()
{
   struct Relationship
   {
      QString sha;
      QStringList parents;
   };

   /* This creates the following graph (S = Start, E = End):
    * S-O-O---O-O-O----O-O-E
    *   |---O-------O--|
    *       |---------O
    */
   QVector<Relationship> vector;
   vector.append(Relationship { "0000000000000000000000000000000000000000", { "6a5ce03e6aab8184601a7e68daccb4463772c3b8" } });
   vector.append(Relationship { "6a5ce03e6aab8184601a7e68daccb4463772c3b8", { "cd77dc03082595418d26c2c36b376f66562093e9" } });
   vector.append(Relationship { "cd77dc03082595418d26c2c36b376f66562093e9", { "ad86b2e4dc69144eeeb1818906e04980336ef820", "3bb3bea6eaf48249f834fa16c5c4107c9562d5f9" } });
   vector.append(Relationship { "14d71c6ed6db586b19aa260635d678e207cce01c", { "2fb103c1ac7f9ab11922c47b522893ea6efe2b06" } });
   vector.append(Relationship { "3bb3bea6eaf48249f834fa16c5c4107c9562d5f9", { "2fb103c1ac7f9ab11922c47b522893ea6efe2b06" } });
   vector.append(Relationship { "ad86b2e4dc69144eeeb1818906e04980336ef820", { "0af130f65eb19b5d5c9952b9e89b3e30edd03ff8" } });
   vector.append(Relationship { "0af130f65eb19b5d5c9952b9e89b3e30edd03ff8", { "bab2fb76670af2a326ebda7a6ad41105aa1ffef1" } });
   vector.append(Relationship { "bab2fb76670af2a326ebda7a6ad41105aa1ffef1", { "88262659d57fd1dccbf155161af5d20dc31f0f3d" } });
   vector.append(Relationship { "2fb103c1ac7f9ab11922c47b522893ea6efe2b06", { "90b17b31217912bbaf6b67d85016e49cc35ca222" } });
   vector.append(Relationship { "88262659d57fd1dccbf155161af5d20dc31f0f3d", { "90b17b31217912bbaf6b67d85016e49cc35ca222" } });
   vector.append(Relationship { "90b17b31217912bbaf6b67d85016e49cc35ca222", { "57284a144f785a5607ea3c6152b901946dbd1af1" } });
   vector.append(Relationship { "57284a144f785a5607ea3c6152b901946dbd1af1", {} });

   Graph::TemporalLoom loom;
   QVector<Graph::Timeline> timelines;

   for (auto &v : vector)
      timelines += loom.createTimeline(v.sha, v.parents);

   QVERIFY(timelines.count() == 12);

   auto i = 0;

   QVERIFY(timelines[i].count() == 1);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Branch);
   ++i;

   QVERIFY(timelines[i].count() == 1);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Active);
   ++i;

   QVERIFY(timelines[i].count() == 2);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::MergeForkLeft);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::HeadRight);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::Branch);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Active);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::Inactive);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Active);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::Inactive);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Active);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::Inactive);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Active);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::Inactive);
   ++i;

   QVERIFY(timelines[i].count() == 3);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Inactive);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::MergeForkLeft);
   QCOMPARE(timelines[i].at(2).getType(), Graph::StateType::TailRight);
   ++i;

   QVERIFY(timelines[i].count() == 2);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Active);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::Inactive);
   ++i;

   QVERIFY(timelines[i].count() == 2);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::MergeForkLeft);
   QCOMPARE(timelines[i].at(1).getType(), Graph::StateType::TailRight);
   ++i;

   QVERIFY(timelines[i].count() == 1);
   QCOMPARE(timelines[i].at(0).getType(), Graph::StateType::Initial);
   ++i;
}
