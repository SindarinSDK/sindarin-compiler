// Fib.cs - Fibonacci Benchmark for C#
// Tests function call overhead (recursive) and loop performance (iterative)

using System;
using System.Diagnostics;

class Fib
{
    // Recursive Fibonacci - tests function call overhead
    static long FibRecursive(int n)
    {
        if (n <= 1) return n;
        return FibRecursive(n - 1) + FibRecursive(n - 2);
    }

    // Iterative Fibonacci - tests loop performance
    static long FibIterative(int n)
    {
        if (n <= 1) return n;
        long a = 0, b = 1;
        for (int i = 2; i <= n; i++)
        {
            long temp = a + b;
            a = b;
            b = temp;
        }
        return b;
    }

    static void Main()
    {
        Stopwatch sw = new Stopwatch();
        long result;

        // Recursive fib(35)
        sw.Start();
        result = FibRecursive(35);
        sw.Stop();
        Console.WriteLine($"Recursive fib(35) = {result}");
        Console.WriteLine($"Recursive time: {sw.ElapsedMilliseconds}ms");

        // Iterative fib(50)
        sw.Restart();
        result = FibIterative(50);
        sw.Stop();
        Console.WriteLine($"Iterative fib(50) = {result}");
        Console.WriteLine($"Iterative time: {sw.ElapsedMilliseconds}ms");
    }
}
