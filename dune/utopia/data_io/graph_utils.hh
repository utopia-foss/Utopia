#ifndef DATAIO_GRAPH_UTILS_HH
#define DATAIO_GRAPH_UTILS_HH

#include <boost/graph/properties.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/graph_traits.hpp>

#include <dune/utopia/data_io/hdfgroup.hh>

namespace Utopia {
namespace DataIO {

/// Write function for a static boost::Graph
/**
 * 
 * @tparam true Saves the edges if true 
 * @tparam GraphType 
 * @param g The graph
 * @param parent_grp The parent HDFGroup
 * @param name The name of the graph group
 * @return std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>> The network data in a HDFGroup
 */
template<
    bool save_edges = true,
    typename GraphType>
std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>>
save_graph(GraphType &g,
         const std::shared_ptr<HDFGroup>& parent_grp,
         const std::string& name)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize datasets to store vertices list and adjacency list in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});
    auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(boost::vertex_index_t(), g, vd);}
    );

    // Save adjacency list
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            // Extract indices of source and target vertex as well as of the
            // edge this corresponds to.
            return std::array<std::size_t, 2>( // TODO use boost::index_type?
                {{boost::get(boost::vertex_index_t(), g, boost::source(ed, g)),
                 boost::get(boost::vertex_index_t(), g, boost::target(ed, g))}}
            );
        }
    );

    // Return newly created group
    return grp;
}


/// Write function for a static boost::Graph
/**
 * 
 * @tparam false Saves the edges if true 
 * @tparam GraphType 
 * @param g The graph
 * @param parent_grp The parent HDFGroup
 * @param name The name of the graph group
 * @return std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>> The network data in a HDFGroup
 */
 template<
    bool save_edges = true,
    typename GraphType>
std::enable_if_t<!save_edges, std::shared_ptr<HDFGroup>>
save_graph(GraphType &g,
         const std::shared_ptr<HDFGroup>& parent_grp,
         const std::string& name)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    grp->add_attribute("save_edges", save_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize datasets to store vertices list
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
                   [&](auto vd){return boost::get(boost::vertex_index, g, vd);}
    );


    // Return newly created group
    return grp;
}


/// Write function for a static boost::Graph
/**
 * @tparam true Saves the edges if true 
 * @tparam PropertyMap The property map of the vertex ids
 * @tparam GraphType 
 * @param g The graph
 * @param parent_grp The parent HDFGroup
 * @param name The name of the graph group
 * @return std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>> The network data in a HDFGroup
 */
template<
    bool save_edges = true,
    typename PropertyMap,
    typename GraphType>
std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>>
save_graph(GraphType &g,
         const std::shared_ptr<HDFGroup>& parent_grp,
         const std::string& name,
         const PropertyMap ids)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize datasets to store vertices list and adjacency list in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});
    auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(ids, vd);}
    );

    // Save adjacency list
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            // Extract indices of source and target vertex as well as of the
            // edge this corresponds to.
            return std::array<std::size_t, 2>( // TODO use boost::index_type?
                {boost::get(ids, boost::source(ed, g)),
                 boost::get(ids, boost::target(ed, g))}
            );
        }
    );

    // Return newly created group
    return grp;
}


/// Write function for a static boost::Graph
/**
 * @tparam true Saves the edges if true 
 * @tparam PropertyMap The property map of the vertex ids
 * @tparam GraphType 
 * @param g The graph
 * @param parent_grp The parent HDFGroup
 * @param name The name of the graph group
 * @return std::enable_if_t<save_edges, std::shared_ptr<HDFGroup>> The network data in a HDFGroup
 */
 template<
    bool save_edges = false,
    typename PropertyMap,
    typename GraphType>
std::enable_if_t<!save_edges, std::shared_ptr<HDFGroup>>
save_graph(GraphType &g,
         const std::shared_ptr<HDFGroup>& parent_grp,
         const std::string& name,
         const PropertyMap ids)
{
    // Create the group for the graph
    auto grp = parent_grp->open_group(name);

    // Store in attributes that this is supposed to be a Graph
    grp->add_attribute("is_static_graph_group", true);

    // Collect some metadata
    auto num_vertices = boost::num_vertices(g);
    auto num_edges = boost::num_edges(g);

    // Also store some metadata
    grp->add_attribute("directed", boost::is_directed(g));
    grp->add_attribute("num_vertices", num_vertices);
    grp->add_attribute("num_edges", num_edges);
    // TODO _could_ add more attributes here, should be general though

    // Initialize datasets to store vertices list and adjacency list in
    auto dset_vl = grp->open_dataset("_vertex_list", {num_vertices});
    auto dset_al = grp->open_dataset("_adjacency_list", {num_edges});

    // Save vertices list
    auto [v, v_end] = boost::vertices(g);
    dset_vl->write(v, v_end,
        [&](auto vd){return boost::get(ids, vd);}
    );

    // Save adjacency list
    auto [e, e_end] = boost::edges(g);
    dset_al->write(e, e_end,
        [&](auto ed){
            // Extract indices of source and target vertex as well as of the
            // edge this corresponds to.
            return std::array<std::size_t, 2>( // TODO use boost::index_type?
                {boost::get(ids, boost::source(ed, g)),
                 boost::get(ids, boost::target(ed, g))}
            );
        }
    );

    // Return newly created group
    return grp;
}


} // namespace DataIO
} // namespace Utopia

#endif // DATAIO_GRAPH_UTILS_HH
