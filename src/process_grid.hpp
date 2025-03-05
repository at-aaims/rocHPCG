/**
 * @file process_grid.hpp
 * @author Aditya Kashi
 * @copyright (C) 2025, Oak Ridge National Laboratory, Oak Ridge, TN 37830, USA.
 */

#ifndef ROCHPCG_PROCESS_GRID_H
#define ROCHPCG_PROCESS_GRID_H

#ifndef __HIPCC__
#define __host__
#define __device__
#endif

#include <array>

#include "ComputeOptimalShapeXYZ.hpp"

__host__ __device__
inline
std::array<int, 3> get_3d_from_linear_zyx(const std::array<int, 3>& sizes, const int linear_idx)
{
    std::array<int,3> idx;
    idx[0] = linear_idx / (sizes[1]*sizes[2]); //< z id
    idx[1] = (linear_idx - idx[0]*sizes[1]*sizes[2]) / sizes[2]; //< y id
    idx[2] = linear_idx - idx[0]*sizes[1]*sizes[2] - idx[1]*sizes[2];
    return idx;
}

__host__ __device__
inline
int get_linear_from_3d_zyx(const std::array<int,3>& sizes, const std::array<int,3>& idx)
{
    return idx[0]*sizes[1]*sizes[2] + idx[1]*sizes[2] + idx[2];
}

/*!
 * Assuming a block distribution of ranks across nodes (consecutive ranks fill one node first),
 * a spatially cubical local grid of processes resides on each node rather than a 1D pencil,
 * so as to reduce off-node neighbourhood comms.
 */
class ProcGridLocalCube
{
public:
    __host__ __device__
    ProcGridLocalCube(const std::array<int,3>& n_procs, const int ranks_per_node)
        : nprocs_{n_procs}, ranks_per_node_{ranks_per_node},
        loc_ranks_{get_local_procgrid_size()}
    { }

    /// Get the 3D process coordinates of a given rank
    /** FIXME: nprocs_[i] may not be a multiple of loc_ranks_[i] even on a known system.
     * In this case, once of the "nodes" has less than the regular number of subdomains.
     * The current assumes extra subdomains to fill out all incomplete nodes.
     */
    __host__ __device__
    std::array<int,3> getProcCoords(const int globalrank) const
    {
        const int globalnode = globalrank / ranks_per_node_;
        std::array<int,3> nnodes;
        for(int i = 0; i < 3; i++) {
            nnodes[i] = (nprocs_[i] - 1) / loc_ranks_[i] + 1;
        }
        const int localrank = globalrank % ranks_per_node_;
        const std::array<int,3> node_idx = get_3d_from_linear_zyx(nnodes, globalnode);
        const std::array<int,3> local_idx = get_3d_from_linear_zyx(loc_ranks_, localrank);

        std::array<int,3> gl_pidx;
        for(int i = 0; i < 3; i++) {
            gl_pidx[i] = node_idx[i]*loc_ranks_[i] + local_idx[i];
        }
        return gl_pidx;
    }

    /// Get rank of a process from its 3D process coordinates
    __host__ __device__
    int getRank(const std::array<int,3>& ip) const
    {
        // get node coords of process ip
        std::array<int,3> inode, loc_idx, nnodes;
        for(int i = 0; i < 3; i++) {
            nnodes[i] = (nprocs_[i] - 1) / loc_ranks_[i] + 1;
            inode[i] = ip[i] / loc_ranks_[i];
            loc_idx[i] = ip[i] % loc_ranks_[i];
        }
        const int noderank = get_linear_from_3d_zyx(nnodes, inode);
        const int locrank = get_linear_from_3d_zyx(loc_ranks_, loc_idx);
        return noderank * ranks_per_node_ + locrank;
    }

private:
    const std::array<int,3> nprocs_;    ///< Total number of processes in the 3 coordinate directions
    const int ranks_per_node_;          ///< Number of processes per node
    const std::array<int,3> loc_ranks_; ///< Number of ranks per node in each direction

    __host__ __device__
    std::array<int,3> get_local_procgrid_size() const
    {
        if (ranks_per_node_ == 8) {
            return std::array<int,3>{2, 2, 2};
        } else {
            int lpx, lpy, lpz;
            ComputeOptimalShapeXYZ(ranks_per_node_, lpx, lpy, lpz);
            return std::array<int,3>{lpz, lpy, lpx};
        }
    }
};

#endif
