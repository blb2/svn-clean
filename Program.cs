using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;
using System.Xml.XPath;
using Microsoft.VisualBasic.FileIO;

namespace svn_clean
{
    public class Program
    {
        public static void Main(string[] args)
        {
            bool dryRun = false;
            bool ignoreExternals = false;

            int i;
            for (i = 0; i < args.Length; i++)
            {
                if (!args[i].StartsWith("-"))
                    break;

                if (args[i] == "-n")
                    dryRun = true;
                else if (args[i] == "-x")
                    ignoreExternals = true;
            }

            var dirs = (i < args.Length ? args.Skip(i) : new[] { "." });
            foreach (var dir in dirs)
            {
                try
                {
                    string workingDirectory = Path.GetFullPath(dir);

                    var procInfo = new ProcessStartInfo();
                    procInfo.WorkingDirectory = workingDirectory;
                    procInfo.FileName = "svn";
                    procInfo.Arguments = "status --xml --no-ignore" + (ignoreExternals ? " --ignore-externals" : "");
                    procInfo.UseShellExecute = false;
                    procInfo.RedirectStandardOutput = true;

                    var svn = Process.Start(procInfo);
                    var xml = XDocument.Load(svn.StandardOutput);

                    Debug.Assert(svn.HasExited);

                    var xpath = "/status/target/entry[./wc-status/@item=\"unversioned\" or ./wc-status/@item=\"ignored\"]/@path";
                    var items = from attr in ((IEnumerable)xml.XPathEvaluate(xpath)).OfType<XAttribute>()
                                select Path.Combine(workingDirectory, attr.Value);

                    foreach (var item in items)
                    {
                        bool isDirectory = Directory.Exists(item);

                        if (dryRun)
                        {
                            Console.WriteLine(string.Format("Would remove {0}: {1}", (isDirectory ? "directory" : "file"), item));
                            continue;
                        }

                        try
                        {
                            if (isDirectory)
                            {
                                FileSystem.DeleteDirectory(item, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin, UICancelOption.ThrowException);
                                Console.WriteLine("Removed directory: " + item);
                            }
                            else
                            {
                                FileSystem.DeleteFile(item, UIOption.OnlyErrorDialogs, RecycleOption.SendToRecycleBin, UICancelOption.ThrowException);
                                Console.WriteLine("Removed file: " + item);
                            }
                        }
                        catch (Exception)
                        {
                            Console.Error.WriteLine(string.Format("Error removing {0}: {1}", (isDirectory ? "directory" : "file"), item));
                        }
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine("An exception was thrown: " + e.Message);
                }
            }
        }
    }
}
