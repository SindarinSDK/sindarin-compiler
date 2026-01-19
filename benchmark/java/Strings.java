// Strings.java - String Operations Benchmark for Java
// Tests string concatenation and manipulation performance

public class Strings {
    public static int countOccurrences(String str, String substr) {
        int count = 0;
        int pos = 0;
        while ((pos = str.indexOf(substr, pos)) != -1) {
            count++;
            pos++;
        }
        return count;
    }

    public static void main(String[] args) {
        int count = 100000;
        String hello = "Hello";

        long start = System.currentTimeMillis();

        // Build string using StringBuilder
        StringBuilder builder = new StringBuilder(count * hello.length());
        for (int i = 0; i < count; i++) {
            builder.append(hello);
        }
        String result = builder.toString();

        // Count occurrences of 'llo'
        int occurrences = countOccurrences(result, "llo");

        long end = System.currentTimeMillis();

        System.out.println("String length: " + result.length());
        System.out.println("Occurrences of 'llo': " + occurrences);
        System.out.println("String time: " + (end - start) + "ms");
    }
}
