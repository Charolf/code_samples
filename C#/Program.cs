using System;
using System.IO;
using System.Collections.Concurrent;
using System.Threading;
using System.Diagnostics;

namespace cpe453_lab4_p2
{
    class Program
    {
        private static readonly int ALLDIR = 0;
        private static readonly int THISDIR = 1;
        private static readonly int NAME = 0;
        private static readonly int CONTENT = 1;
        private static readonly int ERROR = 4;
        private static readonly int BUF_SIZE = 2000;

        private static int resultCount = 0;

        static BlockingCollection<string> DirectoriesBC = new
            BlockingCollection<string>(BUF_SIZE);
        static BlockingCollection<string> FilesBC = new
            BlockingCollection<string>(BUF_SIZE);

        static void AddDirToDirBC(string path)
        {
            string[] rawdirs = Directory.GetDirectories(path);
            foreach (string dir in rawdirs)
            {
                DirectoriesBC.Add(dir);
                AddDirToDirBC(dir);
            }
        }

        static void Stage1(string path, int searchOp)
        {
            DirectoriesBC.Add(path);
            if (searchOp == ALLDIR)
                AddDirToDirBC(path);
            DirectoriesBC.CompleteAdding();
        }

        static void Stage2()
        {
            foreach (string path in DirectoriesBC.GetConsumingEnumerable())
            {
                string[] rawfiles = Directory.GetFiles(path);
                foreach (string file in rawfiles)
                    FilesBC.Add(file);
            }
        }

        static void Stage3(string contString, int searchOp)
        {
            foreach (string file in FilesBC.GetConsumingEnumerable())
            {
                if (searchOp == CONTENT)
                    SingleFileContent(file, contString);
                else
                {
                    if (Path.GetFileName(file).Contains(contString))
                    {
                        var finfo = new FileInfo(file);
                        Console.WriteLine("{0}: {1}, {2}", file, finfo.Length,
                         finfo.CreationTime);
                        resultCount++;
                    }
                }
            }
        }

        static void SingleFileContent(string fullpath, string subs)
        {
            int counter = 0;
            var finfo = new FileInfo(fullpath);

            foreach (string line in File.ReadLines(fullpath))
            {
                counter++;
                if (line.Contains(subs))
                {
                    Console.WriteLine("{0}({1}): {2}, {3}", fullpath,
                     counter, finfo.Length, finfo.CreationTime);
                    resultCount++;
                }
            }
        }

        static int CheckArgs(string[] args)
        {
            int counter = 0;
            if (args.Length < 2 || args.Length > 4)
                return ERROR;
            if (args[args.Length - 1][0] == '-' ||
             args[args.Length - 2][0] == '-')
                return ERROR;
            if (args[0] == "-name")
                counter += 2;
            else if (args[0] == "-r")
                counter++;
            else if (args[0][0] == '-')
                return ERROR;
            if (args[1] == args[0] && args[1][0] == '-')
                return ERROR;
            else if (args[1] == "-name")
                counter += 2;
            else if (args[1] == "-r")
                counter++;
            else if (args[1][0] == '-')
                return ERROR;
            return counter;
        }

        // GOOD
        static int Main(string[] args)
        {
            Stopwatch stopWatch = new Stopwatch();
            string searchString = args[args.Length - 2];
            string startingDirectory = args[args.Length - 1];
            int flags = CheckArgs(args);

            Thread[] s2threads = new Thread[1];
            Thread[] s3threads = new Thread[12];

            int dirOp = THISDIR;
            int searchOp = CONTENT;

            if (flags == 4)
            {
                Console.WriteLine("Usage: .\\grep [-name] [-r] SearchString "
                 + "StartingDirectory");
                return 1;
            }
            else if (flags == 1)  // yes traverse, yes content. GOOD
                dirOp = ALLDIR;
            else if (flags == 2)  //  no traverse,  no content. GOOD
                searchOp = NAME;
            else if (flags == 3)  // yes traverse,  no content. GOOD
            {
                dirOp = ALLDIR;
                searchOp = NAME;
            }

            stopWatch.Start();

            Thread stage1 = new Thread(() =>
            {
                Stage1(startingDirectory, dirOp);
            });
            stage1.Start();
            for (int i = 0; i < s2threads.Length; i++)
            {
                s2threads[i] = new Thread(() =>
                {
                    Stage2();
                });
                s2threads[i].Start();
            }
            for (int i = 0; i < s3threads.Length; i++)
            {
                s3threads[i] = new Thread(() =>
                {
                    Stage3(searchString, searchOp);
                });
                s3threads[i].Start();
            }

            stage1.Join();
            foreach (Thread s2thread in s2threads)
                s2thread.Join();
            FilesBC.CompleteAdding();
            foreach (Thread s3thread in s3threads)
                s3thread.Join();

            stopWatch.Stop();

            Console.WriteLine("{0} total matches.", resultCount);
            Console.WriteLine("The search took {0} seconds.",
                stopWatch.ElapsedMilliseconds / 1000.0);

            Console.WriteLine("Hit any key to close...");
            Console.ReadKey();
            return 0;
        }
    }
}
