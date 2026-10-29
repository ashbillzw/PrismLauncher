#include <QTest>
#include <QThread>
#include <QTimer>

#include <tasks/ConcurrentTask.h>
#include <tasks/MultipleOptionsTask.h>
#include <tasks/SequentialTask.h>
#include <tasks/Task.h>

#include <array>

/* Does nothing. Only used for testing. */
class BasicTask : public Task {
    Q_OBJECT

    friend class TaskTest;

   public:
    BasicTask(bool show_debug_log = true) : Task(show_debug_log) {}

   private:
    void executeTask() override { emitSucceeded(); }
};

/* Does nothing. Only used for testing. */
class BasicTask_MultiStep : public Task {
    Q_OBJECT

    friend class TaskTest;

   private:
    auto isMultiStep() const -> bool override { return true; }

    void executeTask() override {}
};

class BigConcurrentTask : public ConcurrentTask {
    Q_OBJECT

    void executeNextSubTask() override
    {
        // This is here only to help fill the stack a bit more quickly (if there's an issue, of course :^))
        // Each tasks thus adds 1024 * 4 bytes to the stack, at the very least.
        [[maybe_unused]] volatile std::array<uint32_t, 1024> some_data_on_the_stack{};

        ConcurrentTask::executeNextSubTask();
    }
};

class BigConcurrentTaskThread : public QThread {
    Q_OBJECT

    QTimer m_deadline;
    void run() override
    {
        BigConcurrentTask big_task;
        m_deadline.setInterval(10000);

        // NOTE: Arbitrary value that manages to trigger a problem when there is one.
        //       Considering each tasks, in a problematic state, adds 1024 * 4 bytes to the stack,
        //       this number is enough to fill up 16 MiB of stack, more than enough to cause a problem.
        static const unsigned s_num_tasks = 1 << 12;
        for (unsigned i = 0; i < s_num_tasks; i++) {
            auto sub_task = makeShared<BasicTask>(false);
            big_task.addTask(sub_task);
        }

        connect(&big_task, &Task::finished, this, &QThread::quit);
        connect(&m_deadline, &QTimer::timeout, this, [this] {
            passed_the_deadline = true;
            quit();
        });

        if (thread() != QThread::currentThread()) {
            QMetaObject::invokeMethod(this, &BigConcurrentTaskThread::start_timer, Qt::QueuedConnection);
        }
        big_task.run();

        exec();
    }
    void start_timer() { m_deadline.start(); }

   public:
    bool passed_the_deadline = false;
};

class TaskTest : public QObject {
    Q_OBJECT

   private slots:
    void test_WeightedDownloadStages()
    {
        auto metadata = makeShared<BasicTask_MultiStep>();
        auto libraries = makeShared<BasicTask_MultiStep>();
        auto assets = makeShared<BasicTask_MultiStep>();
        SequentialTask downloads;
        downloads.addTask(metadata, 0);
        downloads.addTask(libraries, 1);
        downloads.addTask(assets, 2);
        BasicTask installer;
        installer.propagateFromOther(&downloads, 0.7);
        downloads.start();
        QTRY_VERIFY(metadata->isRunning());
        metadata->setProgressFraction(1);
        QCOMPARE(installer.getProgressFraction(), 0.7);
        metadata->emitSucceeded();
        QTRY_VERIFY(libraries->isRunning());
        libraries->setProgressFraction(0.25);
        QCOMPARE(installer.getProgressFraction(), 0.725);
        libraries->setProgressFraction(0.5);
        QCOMPARE(installer.getProgressFraction(), 0.75);
        libraries->setProgressFraction(0.75);
        QCOMPARE(installer.getProgressFraction(), 0.775);
        libraries->emitSucceeded();
        QTRY_VERIFY(assets->isRunning());
        QCOMPARE(installer.getProgressFraction(), 0.8);
        assets->setProgressFraction(0.5);
        QCOMPARE(installer.getProgressFraction(), 0.9);
        assets->emitSucceeded();
        QTRY_VERIFY(downloads.wasSuccessful());
        QCOMPARE(installer.getProgressFraction(), 1.0);
    }

    void test_FractionalSequentialProgress()
    {
        auto first = makeShared<BasicTask_MultiStep>();
        auto second = makeShared<BasicTask_MultiStep>();
        SequentialTask sequence;
        sequence.addTask(first);
        sequence.addTask(second);
        sequence.start();
        QTRY_VERIFY(first->isRunning());
        first->setProgressFraction(0.5);
        QCOMPARE(sequence.getProgressFraction(), 0.25);
        first->emitSucceeded();
        QTRY_VERIFY(second->isRunning());
        QCOMPARE(sequence.getProgressFraction(), 0.5);
        second->setProgressFraction(0.5);
        QCOMPARE(sequence.getProgressFraction(), 0.75);
        second->emitSucceeded();
        QTRY_VERIFY(sequence.wasSuccessful());
        QCOMPARE(sequence.getProgressFraction(), 1.0);
    }

    void test_FractionalProgressMapping()
    {
        BasicTask_MultiStep source;
        BasicTask wrapper;
        wrapper.propagateFromOther(&source, 0.5);
        QCOMPARE(wrapper.getProgressFraction(), 0.5);
        source.setProgress(1, 4);
        source.setProgressFraction(0.25);
        QCOMPARE(wrapper.getProgressFraction(), 0.625);
        QCOMPARE(wrapper.getProgress(), 1);
        QCOMPARE(wrapper.getTotalProgress(), 4);
        source.setProgressFraction(1);
        QCOMPARE(wrapper.getProgressFraction(), 1.0);
    }

    void test_SetStatus_NoMultiStep()
    {
        BasicTask t;
        QString status{ "test status" };

        t.setStatus(status);

        QCOMPARE(t.getStatus(), status);
        QCOMPARE(t.getStepProgress().isEmpty(), TaskStepProgressList{}.isEmpty());
    }

    void test_SetStatus_MultiStep()
    {
        BasicTask_MultiStep t;
        QString status{ "test status" };

        t.setStatus(status);

        QCOMPARE(t.getStatus(), status);
        // Even though it is multi step, it does not override the getStepStatus method,
        // so it should remain the same.
        QCOMPARE(t.getStepProgress().isEmpty(), TaskStepProgressList{}.isEmpty());
    }

    void test_SetProgress()
    {
        BasicTask t;
        int current = 42;
        int total = 207;

        t.setProgress(current, total);

        QCOMPARE(t.getProgress(), current);
        QCOMPARE(t.getTotalProgress(), total);
    }

    void test_basicRun()
    {
        BasicTask t;
        connect(&t, &Task::finished, &t,
                [&t] { QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been."); });
        t.start();

        QVERIFY2(QTest::qWaitFor([&t]() { return t.isFinished(); }, 1000), "Task didn't finish as it should.");
    }

    void test_basicConcurrentRun()
    {
        auto t1 = makeShared<BasicTask>();
        auto t2 = makeShared<BasicTask>();
        auto t3 = makeShared<BasicTask>();

        ConcurrentTask t;

        t.addTask(t1);
        t.addTask(t2);
        t.addTask(t3);

        connect(&t, &Task::finished, &t, [&t, &t1, &t2, &t3] {
            QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
            QVERIFY(t1->wasSuccessful());
            QVERIFY(t2->wasSuccessful());
            QVERIFY(t3->wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&t]() { return t.isFinished(); }, 1000), "Task didn't finish as it should.");
    }

    // Tests if starting new tasks after the 6 initial ones is working
    void test_moreConcurrentRun()
    {
        auto t1 = makeShared<BasicTask>();
        auto t2 = makeShared<BasicTask>();
        auto t3 = makeShared<BasicTask>();
        auto t4 = makeShared<BasicTask>();
        auto t5 = makeShared<BasicTask>();
        auto t6 = makeShared<BasicTask>();
        auto t7 = makeShared<BasicTask>();
        auto t8 = makeShared<BasicTask>();
        auto t9 = makeShared<BasicTask>();

        ConcurrentTask t;

        t.addTask(t1);
        t.addTask(t2);
        t.addTask(t3);
        t.addTask(t4);
        t.addTask(t5);
        t.addTask(t6);
        t.addTask(t7);
        t.addTask(t8);
        t.addTask(t9);

        connect(&t, &Task::finished, &t, [&t, &t1, &t2, &t3, &t4, &t5, &t6, &t7, &t8, &t9] {
            QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
            QVERIFY(t1->wasSuccessful());
            QVERIFY(t2->wasSuccessful());
            QVERIFY(t3->wasSuccessful());
            QVERIFY(t4->wasSuccessful());
            QVERIFY(t5->wasSuccessful());
            QVERIFY(t6->wasSuccessful());
            QVERIFY(t7->wasSuccessful());
            QVERIFY(t8->wasSuccessful());
            QVERIFY(t9->wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&t]() { return t.isFinished(); }, 1000), "Task didn't finish as it should.");
    }

    void test_basicSequentialRun()
    {
        auto t1 = makeShared<BasicTask>();
        auto t2 = makeShared<BasicTask>();
        auto t3 = makeShared<BasicTask>();

        SequentialTask t;

        t.addTask(t1);
        t.addTask(t2);
        t.addTask(t3);

        connect(&t, &Task::finished, &t, [&t, &t1, &t2, &t3] {
            QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
            QVERIFY(t1->wasSuccessful());
            QVERIFY(t2->wasSuccessful());
            QVERIFY(t3->wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&t]() { return t.isFinished(); }, 1000), "Task didn't finish as it should.");
    }

    void test_basicMultipleOptionsRun()
    {
        auto t1 = makeShared<BasicTask>();
        auto t2 = makeShared<BasicTask>();
        auto t3 = makeShared<BasicTask>();

        MultipleOptionsTask t;

        t.addTask(t1);
        t.addTask(t2);
        t.addTask(t3);

        connect(&t, &Task::finished, &t, [&t, &t1, &t2, &t3] {
            QVERIFY2(t.wasSuccessful(), "Task finished but was not successful when it should have been.");
            QVERIFY(t1->wasSuccessful());
            QVERIFY(!t2->wasSuccessful());
            QVERIFY(!t3->wasSuccessful());
        });

        t.start();
        QVERIFY2(QTest::qWaitFor([&t]() { return t.isFinished(); }, 1000), "Task didn't finish as it should.");
    }

    void test_stackOverflowInConcurrentTask()
    {
        BigConcurrentTaskThread thread;

        thread.start();

        QVERIFY2(QTest::qWaitFor([&thread] { return thread.isFinished(); }, 15000), "Thread didn't finish as it should.");

        QVERIFY(!thread.passed_the_deadline);
    }
};

QTEST_GUILESS_MAIN(TaskTest)

#include "Task_test.moc"
