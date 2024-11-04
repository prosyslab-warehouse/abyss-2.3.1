#include "Graph/Path.h"
#include "Graph/ExtendPath.h"
#include <boost/graph/adjacency_list.hpp>
#include <gtest/gtest.h>

using namespace std;

typedef boost::adjacency_list<boost::vecS, boost::vecS,
	boost::bidirectionalS> Graph;

// note: vertex_descriptor for adjacency_list<> is int
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
// note: edge_descriptor for adjacency_list<> is int
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

TEST(extendPath, lookAhead)
{
	unsigned depth;

	/* case 1: simple path */

	/*
	 * 0--1--2
	 */

	Graph g1;
	add_edge(0, 1, g1);
	add_edge(1, 2, g1);

	depth = 1;
	ASSERT_TRUE(lookAhead(1, FORWARD, depth, g1));
	ASSERT_TRUE(lookAhead(1, REVERSE, depth, g1));
	ASSERT_FALSE(lookAhead(2, FORWARD, depth, g1));
	ASSERT_FALSE(lookAhead(0, REVERSE, depth, g1));

	depth = 2;
	ASSERT_FALSE(lookAhead(1, FORWARD, depth, g1));
	ASSERT_FALSE(lookAhead(1, REVERSE, depth, g1));
	ASSERT_TRUE(lookAhead(0, FORWARD, depth, g1));
	ASSERT_TRUE(lookAhead(2, REVERSE, depth, g1));

	/* case 2: with branching */

	/*
	 *      2
	 *     /
	 * 0--1
	 *     \
	 *      3--4
	 */

	Graph g2;
	add_edge(0, 1, g2);
	add_edge(1, 2, g2);
	add_edge(1, 3, g2);
	add_edge(3, 4, g2);

	depth = 3;
	ASSERT_TRUE(lookAhead(0, FORWARD, depth, g2));

	depth = 4;
	ASSERT_FALSE(lookAhead(0, FORWARD, depth, g2));
}

TEST(extendPath, depth)
{
	/*
	 *      2
	 *     /
	 * 0--1
	 *     \
	 *      3--4
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(1, 3, g);
	add_edge(3, 4, g);

	/* note: depth of starting node is 0 */
	ASSERT_EQ(3u, depth(0, FORWARD, g));
	ASSERT_EQ(2u, depth(1, FORWARD, g));
	ASSERT_EQ(3u, depth(4, REVERSE, g));
	ASSERT_EQ(1u, depth(1, REVERSE, g));
}

TEST(extendPath, longestBranch)
{
	/*
	 *      2
	 *     /
	 * 0--1
	 *     \
	 *      3--4
	 *     /
	 *    5
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(1, 3, g);
	add_edge(3, 4, g);
	add_edge(5, 3, g);

	ASSERT_EQ(1u, longestBranch(0, FORWARD, g).first);
	ASSERT_EQ(3u, longestBranch(1, FORWARD, g).first);
	ASSERT_EQ(1u, longestBranch(3, REVERSE, g).first);
	ASSERT_EQ(3u, longestBranch(4, REVERSE, g).first);
}

TEST(extendPath, noExtension)
{
	// Graph containing a single edge.

	Graph g;
	add_edge(0, 1, g);
	Path<Vertex> path;
	path.push_back(0);
	path.push_back(1);

	extendPath(path, FORWARD, g);
	ASSERT_EQ(2u, path.size());

	extendPath(path, REVERSE, g);
	ASSERT_EQ(2u, path.size());
}

TEST(extendPath, extendForward)
{
	/*
	 *      2
	 *     /
	 * 0--1
	 *     \
	 *      3
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(1, 3, g);

	Path<Vertex> expectedPath;
	expectedPath.push_back(0);
	expectedPath.push_back(1);

	Path<Vertex> path;
	path.push_back(0);
	ASSERT_EQ(1u, path.size());

	extendPath(path, FORWARD, g);
	ASSERT_EQ(2u, path.size());
	ASSERT_EQ(expectedPath, path);
}

TEST(extendPath, extendReverse)
{
	/*
	 *  0
	 *   \
	 *    2--3
	 *   /
	 *  1
	 */

	Graph g;
	add_edge(0, 2, g);
	add_edge(1, 2, g);
	add_edge(2, 3, g);

	Path<Vertex> expectedPath;
	expectedPath.push_back(2);
	expectedPath.push_back(3);

	Path<Vertex> path;
	path.push_back(3);
	ASSERT_EQ(1u, path.size());

	extendPath(path, REVERSE, g);
	ASSERT_EQ(2u, path.size());
	ASSERT_EQ(expectedPath, path);
}

TEST(extendPath, bidirectional)
{
	/*
	 *  0         5
	 *   \       /
	 *    2--3--4
	 *   /       \
	 *  1         6
	 */

	Graph g;
	add_edge(0, 2, g);
	add_edge(1, 2, g);
	add_edge(2, 3, g);
	add_edge(3, 4, g);
	add_edge(4, 5, g);
	add_edge(4, 6, g);

	Path<Vertex> expectedPath;
	expectedPath.push_back(2);
	expectedPath.push_back(3);
	expectedPath.push_back(4);

	Path<Vertex> path;
	path.push_back(3);
	ASSERT_EQ(1u, path.size());

	extendPath(path, FORWARD, g);
	extendPath(path, REVERSE, g);
	EXPECT_EQ(3u, path.size());
	ASSERT_EQ(expectedPath, path);
}

TEST(extendPath, withTrimming)
{
	ExtendPathParams params;
	params.trimLen = 1;
	params.fpTrim = 0;

	/*
	 *          3
	 *         /
	 *  0--1--2--4--5
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(2, 3, g);
	add_edge(2, 4, g);
	add_edge(4, 5, g);

	Path<Vertex> expectedPath;
	expectedPath.push_back(0);
	expectedPath.push_back(1);
	expectedPath.push_back(2);
	expectedPath.push_back(4);
	expectedPath.push_back(5);

	Path<Vertex> pathFwd;
	pathFwd.push_back(0);

	extendPath(pathFwd, FORWARD, g, params);
	ASSERT_EQ(expectedPath, pathFwd);

	Path<Vertex> pathRev;
	pathRev.push_back(5);

	extendPath(pathRev, REVERSE, g, params);
	ASSERT_EQ(expectedPath, pathRev);

	/*
	 *       2  4
	 *      /  /
	 *  0--1--3
	 *         \
	 *          5
	 */

	Graph g2;
	add_edge(0, 1, g2);
	add_edge(1, 2, g2);
	add_edge(1, 3, g2);
	add_edge(3, 4, g2);
	add_edge(3, 5, g2);

	Path<Vertex> path2;
	path2.push_back(0);

	extendPath(path2, FORWARD, g2, params);

	/**
	 * Note: In situations where there are
	 * multiple branches shorter than the trim
	 * length, we first check for a unique
	 * branch that is longer than the false positive
	 * trim length (`fpTrim`). If there are multiple branches
	 * longer than the false positive trim length,
	 * they are all considered sequencing error branches
	 * and are all trimmed.  If all branches are shorter
	 * than the false positive trim length, we choose the
	 * longest branch, provided that the choice is
	 * unambiguous (i.e. no ties).
	 */
	ASSERT_EQ(3u, path2.size());
	ASSERT_EQ(0u, path2.at(0));
	ASSERT_EQ(1u, path2.at(1));
	ASSERT_EQ(3u, path2.at(2));
}

TEST(extendPath, trueBranch)
{
	const unsigned trim = 1;
	const unsigned fpTrim = 1;

	/*
	 * This "X" structure is created frequently
	 * by Bloom filter false positives. (The "*"'s
	 * denote the positions of the false
	 * positives.)
	 *
	 *    5
	 *    |
	 * 3* 4
	 * |\/|
	 * |/\|
	 * 1  2*
	 * |
	 * 0
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 3, g);
	add_edge(2, 3, g);
	add_edge(2, 4, g);
	add_edge(4, 5, g);

	ASSERT_FALSE(trueBranch(edge(1, 3, g).first, FORWARD, g, trim, fpTrim));
	ASSERT_TRUE(trueBranch(edge(1, 4, g).first, FORWARD, g, trim, fpTrim));
}

TEST(extendPath, cycles)
{
	PathExtensionResult result;

	/*
	 * 2---1
	 *  \ /
	 *   0
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(2, 0, g);

	Path<Vertex> pathForward;
	pathForward.push_back(0);

	Path<Vertex> expectedPathForward;
	expectedPathForward.push_back(0);
	expectedPathForward.push_back(1);
	expectedPathForward.push_back(2);

	result = extendPath(pathForward, FORWARD, g);
	EXPECT_EQ(2u, result.first);
	EXPECT_EQ(ER_CYCLE, result.second);
	EXPECT_EQ(expectedPathForward, pathForward);

	Path<Vertex> pathReverse;
	pathReverse.push_back(0);

	Path<Vertex> expectedPathReverse;
	expectedPathReverse.push_back(1);
	expectedPathReverse.push_back(2);
	expectedPathReverse.push_back(0);

	result = extendPath(pathReverse, REVERSE, g);
	EXPECT_EQ(2u, result.first);
	EXPECT_EQ(ER_CYCLE, result.second);
	EXPECT_EQ(expectedPathReverse, pathReverse);

	/*
	 *   3---2
	 *    \ /
	 * 0---1
	 */

	Graph g2;
	add_edge(0, 1, g2);
	add_edge(1, 2, g2);
	add_edge(2, 3, g2);
	add_edge(3, 1, g2);

	Path<Vertex> path2;
	path2.push_back(0);

	Path<Vertex> expectedPath2;
	expectedPath2.push_back(0);
	expectedPath2.push_back(1);

	result = extendPath(path2, FORWARD, g2);
	/*
	 * note: expected result is EXTENDED_TO_BRANCHING_POINT
	 * because vertex 1 has 2 incoming edges
	 */
	EXPECT_EQ(1u, result.first);
	EXPECT_EQ(ER_AMBI_IN, result.second);
	EXPECT_EQ(expectedPath2, path2);

	/*
	 * 2---3
	 *  \ /
	 *   1---0
	 */

	Graph g3;
	add_edge(1, 0, g3);
	add_edge(2, 1, g3);
	add_edge(3, 2, g3);
	add_edge(1, 3, g3);

	Path<Vertex> path3;
	path3.push_back(0);

	Path<Vertex> expectedPath3;
	expectedPath3.push_back(1);
	expectedPath3.push_back(0);

	result = extendPath(path3, REVERSE, g3);
	/*
	 * note: expected result is EXTENDED_TO_BRANCHING_POINT
	 * because vertex 1 has 2 incoming edges
	 */
	EXPECT_EQ(1u, result.first);
	EXPECT_EQ(ER_AMBI_IN, result.second);
	EXPECT_EQ(expectedPath3, path3);
}

TEST(extendPath, cyclesAndBranches)
{
	PathExtensionResult result;

	/*
	 *     2
	 *    //
	 * 0--1--3--4
	 */

	Graph g;
	add_edge(0, 1, g);
	add_edge(1, 2, g);
	add_edge(2, 1, g);
	add_edge(1, 3, g);
	add_edge(3, 4, g);

	Path<Vertex> path;
	path.push_back(0);

	Path<Vertex> expectedPath;
	expectedPath.push_back(0);
	expectedPath.push_back(1);

	result = extendPath(path, FORWARD, g);
	EXPECT_EQ(1u, result.first);
	EXPECT_EQ(ER_AMBI_IN, result.second);
	EXPECT_EQ(expectedPath, path);
}
