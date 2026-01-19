// Fib.java - Fibonacci Benchmark for Java
// Tests function call overhead (recursive) and loop performance (iterative)

public class Fib {
    // Recursive Fibonacci - tests function call overhead
    public static long fibRecursive(int n) {
        if (n <= 1) return n;
        return fibRecursive(n - 1) + fibRecursive(n - 2);
    }

    // Iterative Fibonacci - tests loop performance
    public static long fibIterative(int n) {
        if (n <= 1) return n;
        long a = 0, b = 1;
        for (int i = 2; i <= n; i++) {
            long temp = a + b;
            a = b;
            b = temp;
        }
        return b;
    }

    public static void main(String[] args) {
        long start, end, result;

        // Recursive fib(35)
        start = System.currentTimeMillis();
        result = fibRecursive(35);
        end = System.currentTimeMillis();
        System.out.println("Recursive fib(35) = " + result);
        System.out.println("Recursive time: " + (end - start) + "ms");

        // Iterative fib(50)
        start = System.currentTimeMillis();
        result = fibIterative(50);
        end = System.currentTimeMillis();
        System.out.println("Iterative fib(50) = " + result);
        System.out.println("Iterative time: " + (end - start) + "ms");
    }
}
