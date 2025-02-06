
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

#ifndef GENERATEGEOMETRY_HPP
#define GENERATEGEOMETRY_HPP

#include "Geometry.hpp"
#include <array>

void GenerateGeometry(int size, int rank, int numThreads, int pz, local_int_t zl, local_int_t zu, local_int_t nx, local_int_t ny, local_int_t nz, int npx, int npy, int npz, Geometry * geom);

std::array<int,3> getProcIDsNodeLocalCubeGrid(const int globalrank, const std::array<int,3> np,
                                              const int npernode);

#endif // GENERATEGEOMETRY_HPP
