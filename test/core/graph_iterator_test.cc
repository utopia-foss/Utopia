#define BOOST_TEST_MODULE graph iterator_pair Iteratetest

#include <random>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/random.hpp>

#include <utopia/core/graph/iterator.hh>

namespace Utopia{

// -- Type definitions --------------------------------------------------------

/// A test node
struct Node {
    /// A test parameter
    double param;
};

/// The edge container type
using EdgeContType = boost::listS;

/// Tye vertex container type
using VertexContType = boost::vecS;

/// Test Graph
using Graph = boost::adjacency_list<
                    EdgeContType,
                    VertexContType,
                    boost::undirectedS,
                    Node>;

// -- Fixtures ----------------------------------------------------------------

// Test random Graph
struct TestGraph {
    Graph g;
    
    TestGraph(){
        const unsigned num_vertices = 10;
        const unsigned num_edges = 20;
        std::mt19937 rng{};
        constexpr bool allow_parallel = false;
        constexpr bool allow_self_edges = false;

        boost::generate_random_graph(g, 
                                 num_vertices, 
                                 num_edges, 
                                 rng, 
                                 allow_parallel, 
                                 allow_self_edges); 
    }
};


// -- Actual test -------------------------------------------------------------

// Test that the iteration over all possible graph entities works correctly
BOOST_FIXTURE_TEST_CASE(get_iterator_pair, TestGraph)
{
    // .. Vertices
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::vertices>(g);
        for (auto [v, v_end] = boost::vertices(g); v!=v_end; ++v){
            BOOST_TEST(*it == *v);
            ++it;
        }
    }

    // .. Edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::edges>(g);
        for (auto [e, e_end] = boost::edges(g); e!=e_end; ++e){
            BOOST_TEST(*it == *e);
            ++it;
        }
    }

    // .. Neighbors
    using VertexDesc = boost::graph_traits<Graph>::vertex_descriptor;
    VertexDesc v = 2;

    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::neighbors>(v, g);
        for (auto [nb, nb_end] = boost::adjacent_vertices(v, g); nb!=nb_end; ++nb){
            BOOST_TEST(*it == *nb);
            ++it;
        }
    }

    // .. inverse Neighbors
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::neighbors>(v, g);
        for (auto [nb, nb_end] = boost::inv_adjacent_vertices(v, g); nb!=nb_end; ++nb){
            BOOST_TEST(*it == *nb);
            ++it;
        }
    }

    // .. in edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::in_edges>(v, g);
        for (auto [e, e_end] = boost::in_edges(v, g); e!=e_end; ++e){
            BOOST_TEST(*it == *e);
            ++it;
        }
    }

    // .. out edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = GraphUtil::iterator_pair<IterateOver::out_edges>(v, g);
        for (auto [e, e_end] = boost::out_edges(v, g); e!=e_end; ++e){
            BOOST_TEST(*it == *e);
            ++it;
        }
    }
}


// Test that the iteration with ranges over all possible graph entities works correctly
BOOST_FIXTURE_TEST_CASE(get_range, TestGraph)
{
    // .. Vertices
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::vertices(g);
        for (auto v : range<IterateOver::vertices>(g)){
            BOOST_TEST(*it == v);
            ++it;
        }
    }

    // .. Edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::edges(g);
        for (auto e : range<IterateOver::edges>(g)){
            BOOST_TEST(*it == e);
            ++it;
        }
    }

    // .. Neighbors
    using VertexDesc = boost::graph_traits<Graph>::vertex_descriptor;
    VertexDesc v = 2;

    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::adjacent_vertices(v, g);
        for (auto nb : range<IterateOver::neighbors>(v, g)) {
            BOOST_TEST(*it == nb);
            ++it;
        }
    }

    // .. inverse Neighbors
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::inv_adjacent_vertices(v, g);
        for (auto nb : range<IterateOver::inv_neighbors>(v, g)) {
            BOOST_TEST(*it == nb);
            ++it;
        }
    }

    // .. in edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::in_edges(v, g);
        for (auto e : range<IterateOver::in_edges>(v, g)){
            BOOST_TEST(*it == e);
            ++it;
        }
    }

    // .. out edges
    {
        [[maybe_unused]] // because it_end is not used 
        auto [it, it_end] = boost::out_edges(v, g);
        for (auto e : range<IterateOver::out_edges>(v, g)){
            BOOST_TEST(*it == e);
            ++it;
        }
    }
}

} // namespace Utopia::Models::Hierarnet