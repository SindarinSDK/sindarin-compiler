// Arrays.cs - Array Operations Benchmark for C#
// Tests dynamic array operations and iteration

using System;
using System.Diagnostics;

class Arrays
{
    static void Main()
    {
        int size = 1000000;
        Stopwatch sw = new Stopwatch();

        sw.Start();

        // Create array with 1,000,000 integers (0 to 999,999)
        int[] arr = new int[size];
        for (int i = 0; i < size; i++)
        {
            arr[i] = i;
        }

        // Sum all elements
        long sum1 = 0;
        for (int i = 0; i < size; i++)
        {
            sum1 += arr[i];
        }

        // Reverse the array in-place
        for (int i = 0; i < size / 2; i++)
        {
            int temp = arr[i];
            arr[i] = arr[size - 1 - i];
            arr[size - 1 - i] = temp;
        }

        // Sum again to verify
        long sum2 = 0;
        for (int i = 0; i < size; i++)
        {
            sum2 += arr[i];
        }

        sw.Stop();

        Console.WriteLine($"Sum: {sum1}");
        Console.WriteLine($"Reversed sum: {sum2}");
        Console.WriteLine($"Array time: {sw.ElapsedMilliseconds}ms");
    }
}
