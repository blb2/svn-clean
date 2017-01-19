using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace svn_clean
{
    public class Program
    {
        public static void Main(string[] args)
        {
            string[] dirs = (args.Length > 0 ? args : new[] { "." });

            foreach (var dir in dirs)
            {
                try
                {
                }
                catch (Exception e)
                {
                    Console.WriteLine("An exception was thrown: " + e.Message);
                }
            }
        }
    }
}
