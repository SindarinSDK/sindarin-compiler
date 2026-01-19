// Strings.cs - String Operations Benchmark for C#
// Tests string concatenation and manipulation performance

using System;
using System.Diagnostics;
using System.Text;

class Strings
{
    static int CountOccurrences(string str, string substr)
    {
        int count = 0;
        int pos = 0;
        while ((pos = str.IndexOf(substr, pos)) != -1)
        {
            count++;
            pos++;
        }
        return count;
    }

    static void Main()
    {
        int count = 100000;
        string hello = "Hello";
        Stopwatch sw = new Stopwatch();

        sw.Start();

        // Build string using StringBuilder
        StringBuilder builder = new StringBuilder(count * hello.Length);
        for (int i = 0; i < count; i++)
        {
            builder.Append(hello);
        }
        string result = builder.ToString();

        // Count occurrences of 'llo'
        int occurrences = CountOccurrences(result, "llo");

        sw.Stop();

        Console.WriteLine($"String length: {result.Length}");
        Console.WriteLine($"Occurrences of 'llo': {occurrences}");
        Console.WriteLine($"String time: {sw.ElapsedMilliseconds}ms");
    }
}
