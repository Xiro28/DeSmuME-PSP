#include "pspvfpu.h"

float sceFpuRsqrt(float fs) {

}

/*
float sceFpuSqrt(float x) {
	float result;
	__asm__ volatile (
		"mtv     %1, S000\n"
		"vsqrt.s S000, S000\n"
		"mfv     %0, S000\n"
		: "=r"(result) : "r"(x));
	return result;
}*/