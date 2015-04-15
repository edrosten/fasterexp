#include <iostream>
#include <random>
#include <chrono>
#include <cmath>
#include <cstdint>
using namespace std;
using namespace std::chrono;

////////////////////////////////////////////////////////////////////////////////
//
// Test rig
//

//Marsagalia's excellent xorshift PRNG
uint32_t x = 1 , y = 0, z  = 0, w = 0;
uint32_t xorshift128(void) {
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

mt19937 rng;
uniform_real_distribution<float> unif(0, 10);

float rngf()
{
	return xorshift128() * (10.f / 4294967296.f);
	//return unif(rng);
}
 

double dummy=0;

template<class C>
double time_func(const C& f, const string& str, double ns)
{
	int N = 1000000;
	int M = 100;

	double ns_sum=0, ns_sum_square=0;

	high_resolution_clock clock;

	for(int j=0; j < M; j++)
	{
		auto t1 = clock.now();
		
		for(int i=0; i < N; i++)
			dummy += f(-rngf());
		
		auto t2 = clock.now();

		double time_ns = duration_cast<nanoseconds>(t2 - t1).count() *1.0 / N - ns;

		ns_sum += time_ns;
		ns_sum_square += time_ns*time_ns;
	}

	double mean = ns_sum / M;
	double std = sqrt(ns_sum_square/M -mean*mean);

	cout << mean << " +- " << std << "  ns per " << str << " number ";

	return mean;
}



template<class C>
void test_func(const C& f, const string& str, double ns)
{
	time_func(f, str, ns);

	int N = 10000000;
	float err=0;
	for(int i=0; i < N; i++)
	{
		float a = -rngf();
		err = max(err, abs(f(a) - expf(a)));
	}
	
	cout << " max err = " << err << endl;
}


////////////////////////////////////////////////////////////////////////////////
//
// First attempt at exp() using a lookup table and Taylor series
//


constexpr int factorial (int n)
{
    return n > 0 ? n * factorial( n - 1 ) : 1;
}

template<int N>
float taylor_exp(float f)
{
	double accumulate=0;
	for(int n=N; n > 0; n--)
		accumulate = (accumulate + 1.0f/factorial(n)) * f;

	return accumulate + 1;
}



const float exps_0_15[16]=
{
	exp(- 0.f),
	exp(- 1.f),
	exp(- 2.f),
	exp(- 3.f),
	exp(- 4.f),
	exp(- 5.f),
	exp(- 6.f),
	exp(- 7.f),
	exp(- 8.f),
	exp(- 9.f),
	exp(-10.f),
	exp(-11.f),
	exp(-12.f),
	exp(-13.f),
	exp(-14.f),
	exp(-15.f),
};



float exp_taylor_1(float n)
{
	//Input in the range  0 to -large
	//only needs to be accurate relative to 1

	//exp(a+b) = exp(a) exp(b)

	//flt_epsilon = 1e-7
	//and exp(-16) = 1.1254e-07
	//so any exps this small when added to 1 will pretty much vanish.
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n);
	n += fn;
	
	//n is between 0 and 1/16

	return exps_0_15[fn] * taylor_exp<5>(n);
}

////////////////////////////////////////////////////////////////////////////////
//
// Modified first attempt using Taylor series.
//


//see http://lolengine.net/blog/2011/9/17/playing-with-the-cpu-pipeline

template<int N> float int_pow(float x)
{
	return int_pow<N-1>(x) * x;
}

template<> float int_pow<1>(float x)
{
	return x;
}

template<int N> float taylor_exp_silly(float x)
{
	return int_pow<N>(x)*(1.0f/factorial(N)) + taylor_exp_silly<N-1>(x);
}

template<> float taylor_exp_silly<0>(float)
{
	return 1;
}


float exp_taylor_1_silly(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n);
	n += fn;
	
	return exps_0_15[fn] * taylor_exp_silly<5>(n);
}


////////////////////////////////////////////////////////////////////////////////
//
// First attempt with rational functions
// 


float exp_rational_1(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n);
	n += fn;
	n *=0.5;
	
	return exps_0_15[fn] * (1 + n + n*n * (1.f/3.f) ) / (1-n + n*n*(1.f/3.f));	
}


////////////////////////////////////////////////////////////////////////////////
//
// Second attempts at Taylor series and rational functions with larger lookup tables
// 

#include "exp_256.h"

float exp_rational_2(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n*16);
	n += fn *1.f/16;
	n*=0.5f;

	return exps_0_16_256[fn] * (1 + n) / (1-n);

}

float exp_taylor_2(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n*16);
	n += fn *1.f/16;

	return exps_0_16_256[fn] * taylor_exp_silly<2>(n);
}


////////////////////////////////////////////////////////////////////////////////
//
// Double lookup table
//


float exps_0_1[17]=
{
	exp(- 0 / 16.f),
	exp(- 1 / 16.f),
	exp(- 2 / 16.f),
	exp(- 3 / 16.f),
	exp(- 4 / 16.f),
	exp(- 5 / 16.f),
	exp(- 6 / 16.f),
	exp(- 7 / 16.f),
	exp(- 8 / 16.f),
	exp(- 9 / 16.f),
	exp(-10 / 16.f),
	exp(-11 / 16.f),
	exp(-12 / 16.f),
	exp(-13 / 16.f),
	exp(-14 / 16.f),
	exp(-15 / 16.f),
	exp(-1.f)
};


float taylor_exp_double_lookup(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n);
	n += fn;

	//n is between 0 and 1

	int fn2 = -ceil(n*16);
	n += fn2 * 1.f/16;
	
	//n is between 0 and 1/16

	return exps_0_15[fn] * exps_0_1[fn2] * taylor_exp_silly<2>(n);

}




////////////////////////////////////////////////////////////////////////////////
//
// EVAN MOAR TABLES!11!11!
//


float exp_taylor_3(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n*64);
	n += fn *1.f/64;

	return exps_0_16_1024[fn] * taylor_exp_silly<3>(n);
}


float exp_taylor_4(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n*256);
	n += fn *1.f/256;

	return exps_0_16_4096[fn] * taylor_exp_silly<3>(n);
}


float exp_taylor_4a(float n)
{
	if( n <= -16)
		return 0;
	
	int fn = -ceil(n*256);
	n += fn *1.f/256;

	return exps_0_16_4096[fn] * taylor_exp_silly<0>(n);
}



////////////////////////////////////////////////////////////////////////////////
//
// Code taken from the internet
//

/*=====================================================================*
 *                   Copyright (C) 2012 Paul Mineiro                   *
 * All rights reserved.                                                *
 *                                                                     *
 * Redistribution and use in source and binary forms, with             *
 * or without modification, are permitted provided that the            *
 * following conditions are met:                                       *
 *                                                                     *
 *     * Redistributions of source code must retain the                *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer.                                       *
 *                                                                     *
 *     * Redistributions in binary form must reproduce the             *
 *     above copyright notice, this list of conditions and             *
 *     the following disclaimer in the documentation and/or            *
 *     other materials provided with the distribution.                 *
 *                                                                     *
 *     * Neither the name of Paul Mineiro nor the names                *
 *     of other contributors may be used to endorse or promote         *
 *     products derived from this software without specific            *
 *     prior written permission.                                       *
 *                                                                     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND              *
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,         *
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES               *
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE             *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER               *
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,                 *
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES            *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE           *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR                *
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF          *
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY              *
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE             *
 * POSSIBILITY OF SUCH DAMAGE.                                         *
 *                                                                     *
 * Contact: Paul Mineiro <paul@mineiro.com>                            *
 *=====================================================================*/

#define cast_uint32_t static_cast<uint32_t>
static inline float
fastpow2 (float p)
{
  float offset = (p < 0) ? 1.0f : 0.0f;
  float clipp = (p < -126) ? -126.0f : p;
  int w = clipp;
  float z = clipp - w + offset;
  union { uint32_t i; float f; } v = { cast_uint32_t ( (1 << 23) * (clipp + 121.2740575f + 27.7280233f / (4.84252568f - z) - 1.49012907f * z) ) };

  return v.f;
}

static inline float
fastexp (float p)
{
  return fastpow2 (1.442695040f * p);
}

static inline float
fasterpow2 (float p)
{
  float clipp = (p < -126) ? -126.0f : p;
  union { uint32_t i; float f; } v = { cast_uint32_t ( (1 << 23) * (clipp + 126.94269504f) ) };
  return v.f;
}

static inline float
fasterexp (float p)
{
  return fasterpow2 (1.442695040f * p);
}



void time_function()
{
	double rng = time_func([](float f){return f;}, "RNG", 0);
	cout << endl;
	
	test_func(expf, "expf", rng);
	test_func(exp_taylor_1, "exp_taylor_1", rng);
	test_func(exp_taylor_1_silly, "exp_taylor_1_silly", rng);
	test_func(exp_taylor_2, "exp_taylor_2", rng);
	test_func(exp_taylor_3, "exp_taylor_3", rng);
	test_func(exp_taylor_4, "exp_taylor_4", rng);
	test_func(exp_taylor_4a, "exp_taylor_4a", rng);
	test_func(exp_rational_1, "exp_rational_1", rng);
	test_func(exp_rational_2, "exp_rational_2", rng);
	test_func(taylor_exp_double_lookup, "taylor_exp_double_lookup", rng);
	test_func(fastexp, "fastexp", rng);
	test_func(fasterexp, "fasterexp", rng);

	cout << dummy << endl;
}




int main()
{
	time_function();
}
