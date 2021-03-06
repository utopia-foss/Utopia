#ifndef UTOPIA_CORE_GRAPH_ITERATORS_HH
#define UTOPIA_CORE_GRAPH_ITERATORS_HH

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/subgraph.hpp>
#include <boost/graph/filtered_graph.hpp>


namespace Utopia {
/**
 *  \ingroup Graph
 *  \addtogroup GraphIterators
 *  \{
 */


/// Over which graph entity to iterate
enum class IterateOver {
    /// Iterate over vertices.
    vertices,

    /// Iterate over edges.
    edges,

    /// Iterate over neighbors (adjacent_vertices).
    /** This iteration requires a vertex descriptor whose neighbors to iterate
     *  over.
     */
    neighbors,

    /// Iterate inversely over neighbors (adjacent_vertices).
    /** This iteration requires a vertex descriptor whose neighbors to iterate
     *  over.
     */
    inv_neighbors,

    /// Iterate over the in edges of a vertex.
    /** This iteration requires a vertex descriptor whose neighbors to iterate
     *  over.
     */
    in_edges,

    /// Iterate over the out edges of a vertex.
    /** This iteration requires a vertex descriptor whose neighbors to iterate
     *  over.
     */
    out_edges
};


namespace GraphUtils
{

/// Get an iterator pair over selected graph entities
/**
 * \tparam iterate_over Specify over which graph entities to iterate
 *                      Valid options:
 *                          - IterateOver::vertices
 *                          - IterateOver::edges
 * \tparam Graph        The graph type
 *
 * \param g             The graph
 *
 * \return decltype(auto) The iterator pair
 */
template<IterateOver iterate_over, typename Graph>
decltype(auto) iterator_pair(const Graph& g){
    using namespace boost;

    if constexpr (iterate_over == IterateOver::vertices){
        return vertices(g);
    }
    else if constexpr (iterate_over == IterateOver::edges){
        return edges(g);
    }
    else {
        static_assert((iterate_over == IterateOver::vertices)
                      or (iterate_over == IterateOver::edges),
                      "Only the iterator pairs of vertices and edges can be"
                      "returned: iterate_over is set wrong!");
    }
}


/// Get an iterator pair over selected graph entities
/** This function returns the iterator pair wrt. another graph entity.
 *  For example iteration over neighbors (adjacent_vertices) needs a references
 *  vertex.
 *
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options:
 *                          - IterateOver::neighbors
 *                          - IterateOver::inv_neighbors
 *                          - IterateOver::in_edges
 *                          - IterateOver::out_egdes
 * \tparam Graph        The graph type
 * \tparam EntityDesc   The graph entity descriptor that is the reference point
 *                      for the iteration.
 *
 * \param e             The graph entity that serves as reference
 * \param g             The graph
 *
 * \return decltype(auto) The iterator pair
 */
template<IterateOver iterate_over, typename Graph, typename EntityDesc>
decltype(auto) iterator_pair(EntityDesc e, const Graph& g){
    using namespace boost;

    if constexpr (iterate_over == IterateOver::neighbors){
        return adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == IterateOver::inv_neighbors){
        return inv_adjacent_vertices(e, g);
    }
    else if constexpr (iterate_over == IterateOver::in_edges){
        return in_edges(e, g);
    }
    else if constexpr (iterate_over == IterateOver::out_edges){
        return out_edges(e, g);
    }
    else {
        static_assert((iterate_over == IterateOver::neighbors)
                      or (iterate_over == IterateOver::inv_neighbors)
                      or (iterate_over == IterateOver::in_edges)
                      or (iterate_over == IterateOver::out_edges),
                      "Only the iterator pairs of neighbors, inv_neighbors, "
                      "in_edge, and out_edge can be returned: "
                      "iterate_over is set wrong!");
    }
}

} // namespace --- Graph


/// Get the iterator range over selected graph entities
/**
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options:
 *                          - IterateOver::vertices
 *                          - IterateOver::edges
 * \tparam Graph        The graph type
 *
 * \param g             The graph
 *
 * \return decltype(auto) The iterator range
 */
template<IterateOver iterate_over, typename Graph>
decltype(auto) range(const Graph& g){
    return boost::make_iterator_range(
                GraphUtils::iterator_pair<iterate_over>(g));
}


/// Get the iterator range over selected graph entities
/** This function returns the iterator range wrt. another graph entity.
 *  For example iterating of the neighbors (adjacent_vertices) of a vertex
 *  requires a vertex descriptor as reference.
 *
 *
 * \tparam iterate_over Specify over which graph entities to iterate over
 *                      Valid options:
 *                          - IterateOver::neighbors
 *                          - IterateOver::inv_neighbors
 *                          - IterateOver::in_edges
 *                          - IterateOver::out_egdes
 * \tparam Graph        The graph type
 * \tparam EntityDesc   The graph entity descriptor that is the reference point
 *                      for the iteration.
 *
 * \param e             The graph entity that serves as reference
 * \param g             The graph
 *
 * \return decltype(auto) The iterator range
 */
template<IterateOver iterate_over, typename Graph, typename EntityDesc>
decltype(auto) range(EntityDesc e, const Graph& g){
    return boost::make_iterator_range(
                GraphUtils::iterator_pair<iterate_over>(e, g));
}

// end group GraphIterators
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_ITERATORS_HH
