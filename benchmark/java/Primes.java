// Primes.java - Prime Sieve Benchmark for Java
// Tests memory allocation and CPU-bound computation

public class Primes {
    // Sieve of Eratosthenes
    public static int sieveOfEratosthenes(int limit) {
        boolean[] isPrime = new boolean[limit + 1];
        for (int i = 0; i <= limit; i++) {
            isPrime[i] = true;
        }
        isPrime[0] = false;
        isPrime[1] = false;

        for (int i = 2; i * i <= limit; i++) {
            if (isPrime[i]) {
                for (int j = i * i; j <= limit; j += i) {
                    isPrime[j] = false;
                }
            }
        }

        int count = 0;
        for (int i = 2; i <= limit; i++) {
            if (isPrime[i]) count++;
        }
        return count;
    }

    public static void main(String[] args) {
        int limit = 1000000;

        long start = System.currentTimeMillis();
        int count = sieveOfEratosthenes(limit);
        long end = System.currentTimeMillis();

        System.out.println("Primes up to " + limit + ": " + count);
        System.out.println("Sieve time: " + (end - start) + "ms");
    }
}
