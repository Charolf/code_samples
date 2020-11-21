using System;
using System.Threading;
using System.Diagnostics;

namespace cpe453_lab3
{
    class Program
    {
        private void PrintResult(int testNum, int sharedVal, long totalTime)
        {
            if (testNum != 1)
                Console.WriteLine("Main thread joined all created threads.");
            Console.WriteLine("Test complete.");
            Console.WriteLine("Shared Value: {0}, Total Time: {1} ms", 
                sharedVal, totalTime);
            if (testNum == 5)
                Console.Write("\n");
        }

        private void PrintTestTitle(int testNum)
        {
            Console.Write("Starting Test {0} ", testNum);
            if (testNum == 1)
                Console.WriteLine("(No threads)...");
            else if (testNum == 2)
                Console.WriteLine("(Threads without synchronization)...");
            else if (testNum == 3)
                Console.WriteLine("(Threads using a mutex)...");
            else if (testNum == 4)
                Console.WriteLine("(Threads using interlocked)...");
            else
                Console.WriteLine("(Threads using a semaphore)...");
        }

        private void PrintTestCategory(int timeInCS, int timeAfterCS)
        {
            Console.WriteLine("********** Lab3 Thread/Synchronization Testing ("
                + "{0}, {1}) **********\n", timeInCS, timeAfterCS);
        }

        private void SharedIntChanger1(int timeInCS, int timeAfterCS)
        {
            Random rand = new Random();
            Stopwatch stopWatch = new Stopwatch();
            int SharedInt = 0;

            stopWatch.Start();
            for (int i = 0; i < 5000; i++)
            {
                Thread.Sleep(rand.Next(timeInCS));
                SharedInt++;
                Thread.Sleep(rand.Next(timeAfterCS));
            }
            stopWatch.Stop();
            PrintResult(1, SharedInt, stopWatch.ElapsedMilliseconds);
        }

        private void SharedIntChanger2(int timeInCS, int timeAfterCS)
        {
            Random rand = new Random();
            Stopwatch stopWatch = new Stopwatch();
            Thread[] lthread = new Thread[10];
            int SharedInt = 0;

            stopWatch.Start();
            for (int i = 0; i < 10; i++)
            {
                lthread[i] = new Thread(() =>
                {
                    for (int j = 0; j < 500; j++)
                    {
                        Thread.Sleep(rand.Next(timeInCS));
                        SharedInt++;
                        Thread.Sleep(rand.Next(timeAfterCS));
                    }
                });
                lthread[i].Start();
            }
            foreach (Thread thread in lthread)
                thread.Join();
            stopWatch.Stop();
            PrintResult(2, SharedInt, stopWatch.ElapsedMilliseconds);
        }

        private void SharedIntChanger3(int timeInCS, int timeAfterCS)
        {
            Random rand = new Random();
            Stopwatch stopWatch = new Stopwatch();
            Mutex mut = new Mutex();
            Thread[] lthread = new Thread[10];
            int SharedInt = 0;

            stopWatch.Start();
            for (int i = 0; i < 10; i++)
            {
                lthread[i] = new Thread(() =>
                {
                    for (int j = 0; j < 500; j++)
                    {
                        mut.WaitOne();
                        Thread.Sleep(rand.Next(timeInCS));
                        SharedInt++;
                        mut.ReleaseMutex();
                        Thread.Sleep(rand.Next(timeAfterCS));
                    }
                });
                lthread[i].Start();
            }
            foreach (Thread thread in lthread)
                thread.Join();
            stopWatch.Stop();
            PrintResult(3, SharedInt, stopWatch.ElapsedMilliseconds);
        }

        private void SharedIntChanger4(int timeInCS, int timeAfterCS)
        {
            Random rand = new Random();
            Stopwatch stopWatch = new Stopwatch();
            Thread[] lthread = new Thread[10];
            int SharedInt = 0;
            int usingResource = 0;

            stopWatch.Start();
            for (int i = 0; i < 10; i++)
            {
                lthread[i] = new Thread(() =>
                {
                    for (int j = 0; j < 500; j++)
                    {
                        Thread.Sleep(rand.Next(timeInCS));
                        Interlocked.Increment(ref SharedInt);
                        Thread.Sleep(rand.Next(timeAfterCS));
                        Interlocked.Exchange(ref usingResource, 0);
                    }
                });
                lthread[i].Start();
            }
            foreach (Thread thread in lthread)
                thread.Join();
            stopWatch.Stop();
            PrintResult(4, SharedInt, stopWatch.ElapsedMilliseconds);
        }

        private void SharedIntChanger5(int timeInCS, int timeAfterCS)
        {
            Random rand = new Random();
            Stopwatch stopWatch = new Stopwatch();
            Thread[] lthread = new Thread[10];
            Semaphore pool = new Semaphore(1, 1);
            int SharedInt = 0;

            stopWatch.Start();
            for (int i = 0; i < 10; i++)
            {
                lthread[i] = new Thread(() =>
                {
                    for (int j = 0; j < 500; j++)
                    {
                        pool.WaitOne();
                        Thread.Sleep(rand.Next(timeInCS));
                        SharedInt++;
                        pool.Release();
                        Thread.Sleep(rand.Next(timeAfterCS));
                    }
                });
                lthread[i].Start();
            }
            foreach (Thread thread in lthread)
                thread.Join();
            stopWatch.Stop();
            PrintResult(5, SharedInt, stopWatch.ElapsedMilliseconds);
        }

        static void Main(string[] args)
        {
            Program prog = new Program();

            prog.PrintTestCategory(10, 1);
            prog.PrintTestTitle(1);
            prog.SharedIntChanger1(10, 1);
            prog.PrintTestTitle(2);
            prog.SharedIntChanger2(10, 1);
            prog.PrintTestTitle(3);
            prog.SharedIntChanger3(10, 1);
            prog.PrintTestTitle(4);
            prog.SharedIntChanger4(10, 1);
            prog.PrintTestTitle(5);
            prog.SharedIntChanger5(10, 1);

            prog.PrintTestCategory(1, 10);
            prog.PrintTestTitle(1);
            prog.SharedIntChanger1(1, 10);
            prog.PrintTestTitle(2);
            prog.SharedIntChanger2(1, 10);
            prog.PrintTestTitle(3);
            prog.SharedIntChanger3(1, 10);
            prog.PrintTestTitle(4);
            prog.SharedIntChanger4(1, 10);
            prog.PrintTestTitle(5);
            prog.SharedIntChanger5(1, 10);

            prog.PrintTestCategory(10, 10);
            prog.PrintTestTitle(1);
            prog.SharedIntChanger1(10, 10);
            prog.PrintTestTitle(2);
            prog.SharedIntChanger2(10, 10);
            prog.PrintTestTitle(3);
            prog.SharedIntChanger3(10, 10);
            prog.PrintTestTitle(4);
            prog.SharedIntChanger4(10, 10);
            prog.PrintTestTitle(5);
            prog.SharedIntChanger5(10, 10);

            Console.WriteLine("\nHit any key to close...");
            Console.ReadKey();
        }
    }
}
