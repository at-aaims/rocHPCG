
//@HEADER
// ***************************************************
//
// HPCG: High Performance Conjugate Gradient Benchmark
//
// Contact:
// Michael A. Heroux ( maherou@sandia.gov)
// Jack Dongarra     (dongarra@eecs.utk.edu)
// Piotr Luszczek    (luszczek@eecs.utk.edu)
//
// ***************************************************
//@HEADER

/* ************************************************************************
 * Modifications (c) 2019 Advanced Micro Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ************************************************************************ */

/*!
 @file CG.cpp

 HPCG routine
 */

#include <fstream>

#include <cmath>

#ifdef OPT_ROCTX
#include <roctracer/roctx.h>
#endif

#include "hpcg.hpp"

#include "CG.hpp"
#include "mytimer.hpp"
#include "ComputeSPMV.hpp"
#include "ComputeMG.hpp"
#include "ComputeDotProduct.hpp"
#include "ComputeWAXPBY.hpp"


// Use TICK and TOCK to time a code section in MATLAB-like fashion
#ifdef OPT_ROCTX // add roctx in TICK/TOCK
#ifndef HPCG_NO_MPI
#define TICK(x)  hipDeviceSynchronize(); MPI_Barrier(MPI_COMM_WORLD); t0 = mytimer(); roctxRangePush(x) //!< record current time in 't0'
#else
#define TICK(x)  hipDeviceSynchronize(); t0 = mytimer(); roctxRangePush(x) //!< record current time in 't0'
#endif
#define TOCK(t) hipDeviceSynchronize(); roctxRangePop(); t += mytimer() - t0 //!< store time difference in 't' using time in 't0'
#else // don't include markers
#ifndef HPCG_NO_MPI
#define TICK(x)  hipDeviceSynchronize(); MPI_Barrier(MPI_COMM_WORLD); t0 = mytimer() //!< record current time in 't0'
#else
#define TICK(x)  hipDeviceSynchronize(); t0 = mytimer() //!< record current time in 't0'
#endif
#define TOCK(t) hipDeviceSynchronize(); t += mytimer() - t0 //!< store time difference in 't' using time in 't0'
#endif

/*!
  Routine to compute an approximate solution to Ax = b

  @param[in]    geom The description of the problem's geometry.
  @param[inout] A    The known system matrix
  @param[inout] data The data structure with all necessary CG vectors preallocated
  @param[in]    b    The known right hand side vector
  @param[inout] x    On entry: the initial guess; on exit: the new approximate solution
  @param[in]    max_iter  The maximum number of iterations to perform, even if tolerance is not met.
  @param[in]    tolerance The stopping criterion to assert convergence: if norm of residual is <= to tolerance.
  @param[out]   niters    The number of iterations actually performed.
  @param[out]   normr     The 2-norm of the residual vector after the last iteration.
  @param[out]   normr0    The 2-norm of the residual vector before the first iteration.
  @param[out]   times     The 7-element vector of the timing information accumulated during all of the iterations.
  @param[in]    doPreconditioning The flag to indicate whether the preconditioner should be invoked at each iteration.

  @return Returns zero on success and a non-zero value otherwise.

  @see CG_ref()
*/
int CG(const SparseMatrix & A, CGData & data, const Vector & b, Vector & x,
    const int max_iter, const double tolerance, int & niters, double & normr, double & normr0,
    double * times, bool doPreconditioning, bool verbose) {
#ifdef OPT_ROCTX
  roctxRangePush("Total Time");
#endif
  double t_begin = mytimer();  // Start timing right away
  normr = 0.0;
  double rtz = 0.0, oldrtz = 0.0, alpha = 0.0, beta = 0.0, pAp = 0.0;


  double t0 = 0.0, t1 = 0.0, t2 = 0.0, t3 = 0.0, t4 = 0.0, t5 = 0.0;
//#ifndef HPCG_NO_MPI
//  double t6 = 0.0;
//#endif
  local_int_t nrow = A.localNumberOfRows;
  Vector & r = data.r; // Residual vector
  Vector & z = data.z; // Preconditioned residual vector
  Vector & p = data.p; // Direction vector (in MPI mode ncol>=nrow)
  Vector & Ap = data.Ap;

  if (!doPreconditioning && A.geom->rank==0) HPCG_fout << "WARNING: PERFORMING UNPRECONDITIONED ITERATIONS" << std::endl;

#ifdef HPCG_DEBUG
  int print_freq = 1;
  if (print_freq>50) print_freq=50;
  if (print_freq<1)  print_freq=1;
#endif
  // p is of length ncols, copy x to p for sparse MV operation
  HIPCopyVector(x, p);
  TICK("SpMV"); ComputeSPMV(A, p, Ap); TOCK(t3); // Ap = A*p
  TICK("WAXPBY"); ComputeWAXPBY(nrow, 1.0, b, -1.0, Ap, r, A.isWaxpbyOptimized);  TOCK(t2); // r = b - Ax (x stored in p)
  TICK("DDOT"); ComputeDotProduct(nrow, r, r, normr, t4, A.isDotProductOptimized); TOCK(t1);
  normr = sqrt(normr);
#ifdef HPCG_DEBUG
  if (A.geom->rank==0) HPCG_fout << "Initial Residual = "<< normr << std::endl;
#endif
  if(A.geom->rank == 0 && verbose) printf("HIP Initial Residual = %le\n", normr);

  // Record initial residual for convergence testing
  normr0 = normr;

  // Start iterations

  for (int k=1; k<=max_iter && normr/normr0 > tolerance; k++ ) {
    TICK("MG");
    if (doPreconditioning)
      ComputeMG(A, r, z); // Apply preconditioner
    else
      HIPCopyVector (r, z); // copy r to z (no preconditioning)
    TOCK(t5); // Preconditioner apply time

    if (k == 1) {
      TICK("WAXPBY"); ComputeWAXPBY(nrow, 1.0, z, 0.0, z, p, A.isWaxpbyOptimized); TOCK(t2); // Copy Mr to p
      TICK("DDOT"); ComputeDotProduct (nrow, r, z, rtz, t4, A.isDotProductOptimized); TOCK(t1); // rtz = r'*z
    } else {
      oldrtz = rtz;
      TICK("DDOT"); ComputeDotProduct (nrow, r, z, rtz, t4, A.isDotProductOptimized); TOCK(t1); // rtz = r'*z
      beta = rtz/oldrtz;
      TICK("WAXPBY"); ComputeWAXPBY (nrow, 1.0, z, beta, p, p, A.isWaxpbyOptimized);  TOCK(t2); // p = beta*p + z
    }

    TICK("SpMV"); ComputeSPMV(A, p, Ap); TOCK(t3); // Ap = A*p
    TICK("DDOT"); ComputeDotProduct(nrow, p, Ap, pAp, t4, A.isDotProductOptimized); TOCK(t1); // alpha = p'*Ap
    alpha = rtz/pAp;
#ifndef HPCG_REFERENCE
    TICK("WAXPBY"); ComputeFusedWAXPBYDot(nrow, -alpha, Ap, r, normr, t4);
            ComputeWAXPBY(nrow, 1.0, x, alpha, p, x, A.isWaxpbyOptimized); TOCK(t2); // x = x + alpha*p
#else
    TICK("WAXPBY"); ComputeWAXPBY(nrow, 1.0, x, alpha, p, x, A.isWaxpbyOptimized);// x = x + alpha*p
            ComputeWAXPBY(nrow, 1.0, r, -alpha, Ap, r, A.isWaxpbyOptimized);  TOCK(t2);// r = r - alpha*Ap
    TICK("DDOT"); ComputeDotProduct(nrow, r, r, normr, t4, A.isDotProductOptimized); TOCK(t1);
#endif
    normr = sqrt(normr);
#ifdef HPCG_DEBUG
    if (A.geom->rank==0 && (k%print_freq == 0 || k == max_iter))
      HPCG_fout << "Iteration = "<< k << "   Scaled Residual = "<< normr/normr0 << std::endl;
#endif
    if(A.geom->rank == 0 && verbose) printf("HIP Iteration = %d   Scaled Residual = %le\n", k, normr / normr0);
    niters = k;
  }

  // Store times
  times[1] += t1; // dot-product time
  times[2] += t2; // WAXPBY time
  times[3] += t3; // SPMV time
  times[4] += t4; // AllReduce time
  times[5] += t5; // preconditioner apply time
//#ifndef HPCG_NO_MPI
//  times[6] += t6; // exchange halo time
//#endif
#ifdef OPT_ROCTX
  roctxRangePop(); // Total Time
#endif
  times[0] += mytimer() - t_begin;  // Total time. All done...
  return 0;
}
