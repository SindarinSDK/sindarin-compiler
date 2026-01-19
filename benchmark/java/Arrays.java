// Arrays.java - Array Operations Benchmark for Java
// Tests dynamic array operations and iteration

public class Arrays {
    public static void main(String[] args) {
        int size = 1000000;

        long start = System.currentTimeMillis();

        // Create array with 1,000,000 integers (0 to 999,999)
        int[] arr = new int[size];
        for (int i = 0; i < size; i++) {
            arr[i] = i;
        }

        // Sum all elements
        long sum1 = 0;
        for (int i = 0; i < size; i++) {
            sum1 += arr[i];
        }

        // Reverse the array in-place
        for (int i = 0; i < size / 2; i++) {
            int temp = arr[i];
            arr[i] = arr[size - 1 - i];
            arr[size - 1 - i] = temp;
        }

        // Sum again to verify
        long sum2 = 0;
        for (int i = 0; i < size; i++) {
            sum2 += arr[i];
        }

        long end = System.currentTimeMillis();

        System.out.println("Sum: " + sum1);
        System.out.println("Reversed sum: " + sum2);
        System.out.println("Array time: " + (end - start) + "ms");
    }
}
