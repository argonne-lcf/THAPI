
#include <iostream>

#include <gtensor/gtensor.h>

#include "thapi-ctl.h"

// provides convenient shortcuts for common gtensor functions, for example
// underscore ('_') to represent open slice ends.
using namespace gt::placeholders;

/**
 * Templated function for daxpy that can work with host gtensor or device
 * gtensor. Relies on C++11 mandatory copy elision to move the result into the
 * LHS it is assigned to, rather than copying all the data (very important for
 * large arrays). Input arrays are passed by reference, again to avoid copying
 * all the data which would happen by default (just like std::vector, gtensor
 * has copy semantics by default).
 */
template <typename Ex, typename Ey>
auto daxpy(double a, const Ex& x, const Ey& y)
{
  // The expression 'a * x + y' generates a gfunction template, which is an
  // un-evaluated expression. In this case, we force evaluation in the return
  // statementso a new gtensor is created. It can be useful to not add the
  // `gt::eval`, in which case the un-evaluated expression will be returned and
  // can be combined with other operations before being evaluated, or assigned
  // to a gtensor to force immediate evaluation.
  return gt::eval(a * x + y);
}

int main(int argc, char** argv)
{
  int n = 1024 * 1024;
  int nprint = 32;

  double a = 0.5;

  // Define and allocate two 1d vectors of size n on the host. Declare
  // but don't allocate a third 1d host vector for storing the result.
  gt::gtensor<double, 1, gt::space::host> h_x(gt::shape(n));
  gt::gtensor<double, 1, gt::space::host> h_y = gt::empty_like(h_x);
  gt::gtensor<double, 1, gt::space::host> h_axpy;

  // initialize the vectors, x is twice it's index values and y is equal
  // to it's index values. We will perform .5*x + y, so the result should be
  // axpy(i) = 2i.
  for (int i = 0; i < n; i++) {
    h_x(i) = 2.0 * static_cast<double>(i);
    h_y(i) = static_cast<double>(i);
  }

#ifndef GTENSOR_HAVE_DEVICE
#error "device required"
#endif

  // Define and allocate device versions of h_x and h_y, and declare
  // a varaible for the result on gpu.
  gt::gtensor<double, 1, gt::space::device> d_x(gt::shape(n));
  gt::gtensor<double, 1, gt::space::device> d_y = gt::empty_like(d_x);
  gt::gtensor<double, 1, gt::space::device> d_axpy;

  // Explicit copies of input from host to device. Note that this is an
  // overload of the copy function for gtensor and gtensor_span types, not
  // std::copy which has a different signature. The source is the first
  // argument and destination the second argument. Currently thrust::copy is
  // used under the hood in the implementation.
  gt::copy(h_x, d_x);
  gt::copy(h_y, d_y);

  // This automatically generates a computation kernel to run on the
  // device.
  auto expr = a * d_x + d_y;

  for (int i = 0; i < 10; i++) {
    if (i % 2 == 1)
      thapi_start_tracing();

    d_axpy = gt::eval(expr);
    h_axpy = gt::empty_like(h_x);
    gt::copy(d_axpy, h_axpy);

    if (i % 2 == 1)
      thapi_stop_tracing();
  }

  // Define a slice to print a subset of elements for spot checking the
  // result.
  auto print_slice = gt::gslice(_, _, n / nprint);
  std::cout << "a       = " << a << std::endl;
  std::cout << "x       = " << h_x.view(print_slice) << std::endl;
  std::cout << "y       = " << h_y.view(print_slice) << std::endl;
  std::cout << "a*x + y = " << h_axpy.view(print_slice) << std::endl;
}
