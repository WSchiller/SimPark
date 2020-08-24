#include <math.h>
#include <stdlib.h>
#include <omp.h>
#include <cstdio>


#define M_PI	3.14159265359

// Initialize seed for random generator
unsigned int seed = 0;

// Starting state global variables
int	    NowYear;		// 2020 - 2025
int	    NowMonth;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	    NowNumDeer;		// population of deer
int		NowNumCoyote;   // population of coyote

// Global deer population check
int		tempNumDeer;

// Grain constants
const float GRAIN_GROWS_PER_MONTH = 9.0;  // average growth/month inches
const float ONE_DEER_EATS_PER_MONTH = 1.0;

// Precipitaton constants
const float AVG_PRECIP_PER_MONTH = 7.0;	// average ppt./month inches
const float AMP_PRECIP_PER_MONTH = 6.0;	// plus or minus
const float RANDOM_PRECIP = 2.0;	// plus or minus noise

// Temperature constants
const float AVG_TEMP = 60.0;	// average temp fahrenheight
const float AMP_TEMP = 20.0;	// plus or minus
const float RANDOM_TEMP = 10.0;	// plus or minus noise

// Average conditions
const float MIDTEMP = 40.0;
const float MIDPRECIP = 10.0;

// Barrier globals
omp_lock_t	Lock;
int		NumInThreadTeam;
int		NumAtBarrier;
int		NumGone;

// function prototypes
void    GrainDeer();
void    Grain();
void    Watcher();
void	Coyote();
void	InitBarrier(int n);
void    WaitBarrier();
float   SQR(float x);
float   Ranf(unsigned int* seedp, float low, float high);
int     Ranf(unsigned int* seedp, int ilow, int ihigh);
int     rand_r(unsigned int* seed);
void    calcTemp_and_Precip();
void    printMonthResults();


int main() {

	// Calculate starting state temp and precip values  
	calcTemp_and_Precip();

	// Initialize starting state variables
	NowMonth = 0;
	NowYear = 2020; 
	NowNumDeer = 6;
	NowHeight = 18.;
	NowNumCoyote = 3;

	// thread to monitor each section
	omp_set_num_threads(4);	// same as # of sections
	InitBarrier(4);   // Initialize barrier
	#pragma omp parallel sections 
	{	
		// spawn off threads
		#pragma omp section 
		{
			GrainDeer();
		}

		#pragma omp section 
		{
			Grain();
		}

		#pragma omp section 
		{
			Watcher();
		}

		#pragma omp section 
		{
			Coyote();	
		}
	}       // implied barrier -- all functions must return in order
		// to allow any of them to get past here

	return 0;
}

// Computes next number of deer
void GrainDeer() {
	while (NowYear < 2026) {
		
		int tempNextNumDeer = NowNumDeer;

		// Evaluate carrying capcaity, adjust "Next" state
		if (tempNextNumDeer > NowHeight) {
			tempNextNumDeer--;
		}
		else {
			tempNextNumDeer++;
		}

		// DoneComputing barrier, wait for other threads 
		WaitBarrier();
		NowNumDeer = tempNextNumDeer;  

		// DoneAssigning barrier, copy local variables into global
		WaitBarrier();
			
		// DonePrinting barrier, wait for watcher function to print results and move to next month
		WaitBarrier();		
	}
}

// Computes grain height
void Grain() {
	while (NowYear < 2026) {
		
		float tempNextHeight = NowHeight;

		// Calculate current growing conditions
		float tempFactor = exp(-SQR((NowTemp - MIDTEMP) / 10.));
		float precipFactor = exp(-SQR((NowPrecip - MIDPRECIP) / 10.));
		
		tempNextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
		tempNextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
		if (tempNextHeight < 0) tempNextHeight = 0.;
		
		// DoneComputing barrier:
		WaitBarrier();
		NowHeight = tempNextHeight;  // copy local variable height of grain into global

		// DoneAssigning barrier:
		WaitBarrier();
			
		// DonePrinting barrier:
		WaitBarrier(); 			
	}
}

// Tracks months and years
// Prints results for each month
void Watcher() {
	while (NowYear < 2026)
	{
		// DoneComputing barrier:
		WaitBarrier();

		// DoneAssigning barrier:
		WaitBarrier();

		printMonthResults();  
		NowMonth++;  

		// calculate new temperature and preciptation values for next month
		calcTemp_and_Precip();
	
		// Check if new calendar year
		if (NowMonth == 12) {
			NowYear++;
			NowMonth = 0;	
		}

		// DonePrinting barrier:
		WaitBarrier();		
	}
}

// Computes next number of Coyote
void Coyote() {
	while (NowYear < 2026) 
	{
		int tempNextNumDeer = NowNumDeer;
		int tempNextNumCoyote = NowNumCoyote;

		// Coyotes hunt fawns. Fawns born in Spring
		if (NowMonth >= 3 && NowMonth <= 7 && tempNextNumDeer > 0) {

			//check if a fawn was born
			if (tempNumDeer < tempNextNumDeer) {

				int attack = Ranf(&seed, 0, 100);
				if (attack < 50) {
					tempNextNumDeer--; 
					printf("Coyote attacks fawn!\n");
				}
			}
		}
		 tempNumDeer = tempNextNumDeer;

		
		if (NowMonth == 3) {
			int pups = Ranf(&seed, 5, 7) / 2;
			tempNextNumCoyote += pups;
		}

		if (tempNextNumCoyote > 10) {
			tempNextNumCoyote /= 2;  
		}

		if (tempNextNumDeer == 0) {
			tempNextNumCoyote = 3;
		}
			
		// DoneComputing barrier:
		WaitBarrier();

		NowNumDeer = tempNextNumDeer;
		NowNumCoyote = tempNextNumCoyote;
		// DoneAssigning barrier:
		WaitBarrier();

		// DonePrinting barrier:
		WaitBarrier();
	}
}

// initialize barrier, takes in number of threads
void InitBarrier(int n) {
	NumInThreadTeam = n;
	NumAtBarrier = 0;
	omp_init_lock(&Lock);
}

// sets the barrier by locking calling thread
void WaitBarrier() {
	omp_set_lock(&Lock);
	{
		NumAtBarrier++;
		if (NumAtBarrier == NumInThreadTeam)  // release the waiting threads
		{
			NumGone = 0;
			NumAtBarrier = 0;
			// let all other threads return before this one unlocks:
			while (NumGone != NumInThreadTeam - 1);
			omp_unset_lock(&Lock);
			return;
		}
	}
	omp_unset_lock(&Lock);

	while (NumAtBarrier != 0);  // all threads wait here until the last one arrives

	#pragma omp atomic	// sets NumAtBarrier to 0
		NumGone++;
}

// squared exponent
float SQR(float x)
{
	return x * x;
}

// returns random float
float Ranf(unsigned int* seedp, float low, float high) {
	float r = (float) rand_r(seedp); // 0 - RAND_MAX

	return(low + r * (high - low) / (float)RAND_MAX);
}

// returns random integer
int Ranf(unsigned int* seedp, int ilow, int ihigh) {
	float low = (float)ilow;
	float high = (float)ihigh + 0.9999f;

	return (int)(Ranf(seedp, low, high));
}

int rand_r(unsigned int* seed) {
	unsigned int next = *seed;
	int result;
	next *= 1103515245;
	next += 12345;
	result = (unsigned int)(next / 65536) % 2048;
	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;
	next *= 1103515245;
	next += 12345;
	result <<= 10;
	result ^= (unsigned int)(next / 65536) % 1024;
	*seed = next;
	return result;
}

// Calculates temperature and precipitation; stores in global variables NowTemp and NowPrecip
void calcTemp_and_Precip() {
	float ang = (30. * (float)NowMonth + 15.) * (M_PI / 180.);

	float temp = AVG_TEMP - AMP_TEMP * cos(ang);
	NowTemp = temp + Ranf(&seed, -RANDOM_TEMP, RANDOM_TEMP);

	float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin(ang);
	NowPrecip = precip + Ranf(&seed, -RANDOM_PRECIP, RANDOM_PRECIP);
	if (NowPrecip < 0.) NowPrecip = 0.;
}

// Print monthly results
// Convert Temp and Precip to metric units
void printMonthResults() {
	printf("Year: %d  Month: %d  Precip: %.2f  Temp: %.2lf  Grain Height: %.2f  Deer Pop: %d  Coyote Pop: %d\n", NowYear, NowMonth, NowPrecip * 2.54, 5./9. * (NowTemp - 32), NowHeight * 2.54, NowNumDeer, NowNumCoyote);
}

