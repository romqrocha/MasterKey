#include <stdio.h>
#include <math.h>

double my_exp(double exponent, int terms) {
    if (terms < 0) {
        printf("Error: terms must not be negative.\n");
        return 0.0;
    }
    double result = 0.0;
    double quotient = 1.0;
    for (int n = 0; n < terms; n++) {
        result += pow(exponent, n) / quotient;
        quotient *= n + 1;    
    }
    return result;
}



int main() {
    printf("Let's calculate e^x using Taylor series.\n");
    
    double exponent;
    printf("Enter the exponent x: ");
    scanf("%lf", &exponent);
    
    int terms;
    printf("Enter the number of terms to calculate in the Taylor expansion: ");
    scanf("%d", &terms);

    double result = my_exp(exponent, terms);
    printf("e^%.2f is about %.10f (%d terms)\n", exponent, result, terms);
    
    return 0;
}