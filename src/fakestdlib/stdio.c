#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>

char* printfBuffer = 0;

int printf(const char* format, ...) {
	if (!printfBuffer) {
		printfBuffer = malloc(1024);
	}

	va_list va;
	va_start(va, format);

	stbsp_vsprintf(printfBuffer, format, va);
	printString(printfBuffer);

	va_end(va);

	return 0;
}

int fprintf(int fd, const char* format, ...) {
	if (!printfBuffer) {
		printfBuffer = malloc(1024);
	}

	va_list va;
	va_start(va, format);

	stbsp_vsprintf(printfBuffer, format, va);
	if (fd == stderr) {
		printError(printfBuffer);
	} else {
		printString(printfBuffer);
	}

	va_end(va);

	return 0;
}

double strtod(const char* str, char** endptr) {
    double result = 0.0;
    char signedResult = '\0';
    char signedExponent = '\0';
    int decimals = 0;
    int isExponent = 0;
    int hasExponent = 0;
    int hasResult = 0;
    // exponent is logically int but is coded as double so that its eventual
    // overflow detection can be the same as for double result
    double exponent = 0;
    char c;

    for (; '\0' != (c = *str); ++str) {
        if ((c >= '0') && (c <= '9')) {
            int digit = c - '0';
            if (isExponent) {
                exponent = (10 * exponent) + digit;
                hasExponent = 1;
            } else if (decimals == 0) {
                result = (10 * result) + digit;
                hasResult = 1;
            } else {
                result += (double)digit / decimals;
                decimals *= 10;
            }
            continue;
        }

        if (c == '.') {
            if (!hasResult) break; // don't allow leading '.'
            if (isExponent) break; // don't allow decimal places in exponent
            if (decimals != 0) break; // this is the 2nd time we've found a '.'

            decimals = 10;
            continue;
        }

        if ((c == '-') || (c == '+')) {
            if (isExponent) {
                if (signedExponent || (exponent != 0)) break;
                else signedExponent = c;
            } else {
                if (signedResult || (result != 0)) break;
                else signedResult = c;
            }
            continue;
        }

        if (c == 'E') {
            if (!hasResult) break; // don't allow leading 'E'
            if (isExponent) break;
            else isExponent = 1;
            continue;
        }

        break; // unexpected character
    }

    if (isExponent && !hasExponent) {
        while (*str != 'E')
            --str;
    }

    if (!hasResult && signedResult) --str;

    if (endptr) *endptr = (char*)str;

    for (; exponent != 0; --exponent) {
        if (signedExponent == '-') result /= 10;
        else result *= 10;
    }

    if (signedResult == '-' && result != 0) result = -result;

    return result;
}
