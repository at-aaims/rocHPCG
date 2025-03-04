
#include <gtest/gtest.h>
#include "process_grid.hpp"

TEST(NodeLocalGrid, test_better_nodelocal_grid_8_firstnode)
{
    const std::array<int,3> np{16,16,16};
    const int npernode = 8;
    ProcGridLocalCube pg(np, npernode);
    const int rank = 7;
    const auto locpcoords = pg.getProcCoords(rank);
    EXPECT_EQ(locpcoords[0], 1);
    EXPECT_EQ(locpcoords[1], 1);
    EXPECT_EQ(locpcoords[2], 1);
    const auto prank = pg.getRank(locpcoords);
    EXPECT_EQ(prank, rank);
}

TEST(NodeLocalGrid, test_better_nodelocal_grid_8)
{
    const std::array<int,3> np{16,16,16};
    const int npernode = 8;
    ProcGridLocalCube pg(np, npernode);
    const int rank = 597;
    const auto pidx = pg.getProcCoords(rank);
    EXPECT_EQ(pidx[0], 3);
    EXPECT_EQ(pidx[1], 2);
    EXPECT_EQ(pidx[2], 5);
    const auto prank = pg.getRank(pidx);
    EXPECT_EQ(prank, rank);
}

TEST(NodeLocalGrid, test_invertible_better_nodelocal_grid_12)
{
    const std::array<int,3> np{300,203,180};
    const int npernode = 12;
    ProcGridLocalCube pg(np, npernode);
    const int rank = 1597;
    const auto pidx = pg.getProcCoords(rank);
    const auto prank = pg.getRank(pidx);
    EXPECT_EQ(prank, rank);
}

TEST(NodeLocalGrid, test_invertible_better_nodelocal_grid_8_14)
{
    const std::array<int,3> np{7,4,4};
    const int npernode = 8;
    ProcGridLocalCube pg(np, npernode);
    for(int rank = 0; rank < 112; rank++) {
        const auto pidx = pg.getProcCoords(rank);
        const auto prank = pg.getRank(pidx);
        EXPECT_EQ(prank, rank);
    }
}

TEST(NodeLocalGrid, test_better_nodelocal_grid_8_14)
{
    const std::array<int,3> np{7,4,4};
    const int npernode = 8;
    ProcGridLocalCube pg(np, npernode);
    // TODO: Test if a subdomain in the last node-plane (in z-dir) maps to the right rank
}
