/* Copyright (c) 2019 CSC Training */
/* Copyright (c) 2021 ENCCS */
#include <stdio.h>
#include <math.h>

#include "thapi-ctl.h"

#define NX 102400

#define NITER 10

int main(void)
{
  double vecA[NX],vecB[NX],vecC[NX];
  double r=0.2;

/* Initialization of vectors */
  for (int i = 0; i < NX; i++) {
     vecA[i] = pow(r, i);
     vecB[i] = 1.0;
  }

  for (int n = 0; n < NITER; n++) {
    if (n % 2 == 1)
      thapi_ctl_start();

    /* dot product of two vectors */
    #pragma omp target teams distribute parallel for
    for (int i = 0; i < NX; i++) {
       vecC[i] = vecA[i] * vecB[i];
    }

    if (n % 2 == 1)
      thapi_ctl_stop();
  }

  double sum = 0.0;
  /* calculate the sum */
  for (int i = 0; i < NX; i++) {
    sum += vecC[i];
  }
  printf("The sum is: %8.6f \n", sum);
  return 0;
}
