#include "tracing.hpp"

#include <cstdio>
#include <vector>

#include <mpi.h>
#include <roctracer/roctx.h>

struct clock_sync_data {
    std::vector<int> traced_ranks;
};

void clock_sync_event(const clock_sync_data& csd)
{
    int rank{};
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    double payload = static_cast<double>(rank + 1.0);

    roctxRangePush("App_clock_sync");

    // ping
    if (rank == 0) {
        // root; receive messages
        for(auto other_rank : csd.traced_ranks) {
            if(other_rank != 0) {
                MPI_Recv(&payload, 1, MPI_DOUBLE, other_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
    for(auto tr_rank : csd.traced_ranks) {
        if (rank != 0 && rank == tr_rank) {
            // traced rank not root; send message to root
            MPI_Send(&payload, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    // pong 
    for(auto tr_rank : csd.traced_ranks) {
        if (rank != 0 && rank == tr_rank) {
            // traced rank not root; receive message
            MPI_Recv(&payload, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    if (rank == 0) {
        // root; send messages
        for(auto other_rank : csd.traced_ranks) {
            if(other_rank != 0) {
                MPI_Send(&payload, 1, MPI_DOUBLE, other_rank, 1, MPI_COMM_WORLD);
            }
        }
    }

    roctxRangePop();
    
    //MPI_Barrier(MPI_COMM_WORLD);
}

inline int get_rank(const Geometry *const g, const int ipx, const int ipy, const int ipz)
{
    return ipx + ipy*g->npx + ipz*g->npx*g->npy;
}

void do_clock_sync(const Geometry *const g)
{
  clock_sync_data csd;
  const int centre[3] = {g->npx / 2, g->npy / 2, g->npz / 2};
  csd.traced_ranks = std::vector<int>{get_rank(g, centre[0], centre[1], centre[2])};

  if(centre[0]+1 < g->npx) {
      csd.traced_ranks.push_back(get_rank(g, centre[0]+1, centre[1], centre[2]));
  }
  if(centre[1]+1 < g->npy) {
      csd.traced_ranks.push_back(get_rank(g, centre[0], centre[1]+1, centre[2]));
  }
  if(centre[2]+1 < g->npz) {
      csd.traced_ranks.push_back(get_rank(g, centre[0], centre[1], centre[2]+1));
  }
  if(centre[2]-1 >= 0) {
      csd.traced_ranks.push_back(get_rank(g, centre[0], centre[1], centre[2]-1));
  }
  if(centre[1]-1 >= 0) {
      csd.traced_ranks.push_back(get_rank(g, centre[0], centre[1]-1, centre[2]));
  }
  if(centre[0]-1 >= 0) {
      csd.traced_ranks.push_back(get_rank(g, centre[0]-1, centre[1], centre[2]));
  }

  if(g->rank == 0) {
      std::printf("  Tracing ranks:\n    ");
      for(auto &x : csd.traced_ranks) {
          std::printf(" %d,", x);
      }
      std::printf(".\n");
  }
  clock_sync_event(csd);
}
