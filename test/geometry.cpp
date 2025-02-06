
#include <gtest/gtest.h>
#include "GenerateGeometry.hpp"

TEST(NodeLocalGrid, test_better_nodelocal_grid_8_firstnode)
{
    const std::array<int,3> np{16,16,16};
    const int npernode = 8;
    const int rank = 7;
    const auto locpcoords = getProcIDsNodeLocalCubeGrid(rank, np, npernode);
    EXPECT_EQ(locpcoords[0], 1);
    EXPECT_EQ(locpcoords[1], 1);
    EXPECT_EQ(locpcoords[2], 1);
}

TEST(NodeLocalGrid, test_better_nodelocal_grid_8)
{
    const std::array<int,3> np{16,16,16};
    const int npernode = 8;
    const int rank = 597;
    const auto locpcoords = getProcIDsNodeLocalCubeGrid(rank, np, npernode);
    EXPECT_EQ(locpcoords[0], 3);
    EXPECT_EQ(locpcoords[1], 2);
    EXPECT_EQ(locpcoords[2], 5);
}
