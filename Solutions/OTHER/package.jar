package primeFinder;

public class Checker {
    
    public static void main(String[] args) {
        // Plug in an input
        for (int i = 0; i < 1000; i++) {
            
            // Store the function value as n
            int n = (int) (Math.pow(i, 2) + i + 41);
            
            // Check for factors (j) that would prove non-prime
            // j >= 2 because prime can be divisible by 1
            // j < n because prime can be divisible by itself
            for (int j = 2; j < n; j++) {
                
                // Print information if a factor is found
                if (n % j == 0) {
                    System.out.println("i = " + i);
                    System.out.println("n = " + n);
                    System.out.println("j = " + j);
                    System.out.println("");
                } 
            }
            
        }
    }
    
}