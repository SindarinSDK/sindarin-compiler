// Primes.cs - Prime Sieve Benchmark for C#
// Tests memory allocation and CPU-bound computation

using System;
using System.Diagnostics;

class Primes
{
    // Sieve of Eratosthenes
    static int SieveOfEratosthenes(int limit)
    {
        bool[] isPrime = new bool[limit + 1];
        for (int i = 0; i <= limit; i++)
            isPrime[i] = true;
        isPrime[0] = false;
        isPrime[1] = false;

        for (int i = 2; i * i <= limit; i++)
        {
            if (isPrime[i])
            {
                for (int j = i * i; j <= limit; j += i)
                    isPrime[j] = false;
            }
        }

        int count = 0;
        for (int i = 2; i <= limit; i++)
        {
            if (isPrime[i]) count++;
        }
        return count;
    }

    static void Main()
    {
        int limit = 1000000;
        Stopwatch sw = new Stopwatch();

        sw.Start();
        int count = SieveOfEratosthenes(limit);
        sw.Stop();

        Console.WriteLine($"Primes up to {limit}: {count}");
        Console.WriteLine($"Sieve time: {sw.ElapsedMilliseconds}ms");
    }
}
