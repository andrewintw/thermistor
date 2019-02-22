#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define THERMISTOR_NOMINAL	100000	/* 100k */
#define TEMPERATURE_NOMINAL	25		/* 25 degrees C */
#define B_CONSTANT			4250	/* B-constant (K) 25/50 degrees C */

#define BRIDGE_RESISTOR		10000	/* 10k */
#define ADC_RESOLUTION		4095	/* 12bit */

unsigned long resistance(unsigned long raw_adc)

/* function to convert the raw Analog Input reading to a resistance value    
 * Schematic:
 *   [Ground] -- [10,000 ohm bridge resistor] -- | -- [thermistor] --[Vcc (?v)]
 *                                               |
 *                                         Analog Pin
 *
 * For the circuit above:
 * Resistance = ((ADC_RESOLUTION - raw_adc) * BRIDGE_RESISTOR)  / (raw_adc)
 */
{
	float temp;					// temporary variable to store calculations in

	temp = ((ADC_RESOLUTION - raw_adc) * BRIDGE_RESISTOR) / (raw_adc);
	return temp;				// returns the value calculated to the calling function.
}

float steinharthart(unsigned long resistance)
// function users steinhart-hart equation to return a temperature in degrees celsius. 

/*
 * This version utilizes the Steinhart-Hart Thermistor Equation:
 *    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]^3}
 *   for the themistor in the Vernier TMP-BTA probe:
 *    A = 0.7744683326e-3 , B = 2.175287541e-4 and C = 0.4924988799e-7
 *    Using these values should get agreement within 1 degree C to the same probe used with one
 *    of the Vernier interfaces
 * 
 */
{
	float temp;					// temporary variable to store calculations in
	float logRes = log(resistance);

	// calculating logirithms is time consuming for a microcontroller - so we just
	// do this once and store it to a variable.
	float k0 = 0.7744683326e-3;
	float k1 = 2.175287541e-4;
	float k2 = 0.4924988799e-7;

	temp = 1 / (k0 + k1 * logRes + k2 * logRes * logRes * logRes);
	temp = temp - 273.15;		// convert from Kelvin to Celsius 
	return temp;
}

float bconstant(unsigned long resistance)

/* for NCP03WF104F05RL (100k), B-constant (K) 25/50 C is 4250±1%  */
{
	float temp;

	temp = (float) resistance / THERMISTOR_NOMINAL;	// (R/Ro)
	temp = logf(temp);			// ln(R/Ro)
	temp /= B_CONSTANT;			// 1/B * ln(R/Ro)
	temp += 1.0 / (TEMPERATURE_NOMINAL + 273.15);	// + (1/To)
	temp = 1.0 / temp;			// Invert
	temp -= 273.15;				// convert to C

	return temp;
}

int parse_number(const char *nptr)
{
	long num;
	char *endptr;

	num = strtol(nptr, &endptr, 0);
	if (*endptr || !*nptr) {
		fprintf(stderr, "Error: not a number!\n");
		return -1;
	}

	return num;
}

int main(int argc, char *argv[])
{
	unsigned long raw_adc;
	float thermistor, tempSH, tempB;

	if (argc < 2) {
		printf("syntax: %s <raw ADC value>\n", argv[0]);
		exit(1);
	}

	raw_adc = parse_number(argv[1]);

	thermistor = resistance(raw_adc);	// converts raw analog value to a resistance
	printf("Thermistor R:\t%.2f ohm\n", thermistor);

	tempSH = steinharthart(thermistor);	// Applies the Steinhart-hart equation
	tempB = bconstant(thermistor);

	printf("Steinhart-hart:\t%.2f degree C\n", tempSH);
	printf("B-equation:\t%.2f degree C\n", tempB);

	return 0;
}

