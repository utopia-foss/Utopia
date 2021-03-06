#ifndef UTOPIA_CORE_GRIDS_TRIANGULAR_HH
#define UTOPIA_CORE_GRIDS_TRIANGULAR_HH

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using triangular cells
template<class Space>
class TriangularGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr DimType dim = Space::dim;

    /// The type of vectors that have a relation to physical space
    using SpaceVec = typename Space::SpaceVec;

    /// The type of multi-index like arrays, e.g. the grid shape
    using MultiIndex = MultiIndexType<dim>;

    /// The configuration type
    using Config = DataIO::Config;


private:
    // -- TriagonalGrid-specific members --------------------------------------


public:
    // -- Constructors --------------------------------------------------------
    /// Construct a triangular grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    TriangularGrid (std::shared_ptr<Space> space, const Config& cfg)
    :
        Base(space, cfg)
    {}


    // -- Implementations of virtual base class functions ---------------------
    // .. Number of cells & shape .............................................

    /// Number of triangular cells required to fill the physical space
    IndexType num_cells() const override {
        // TODO Implement properly!
        return 0;
    };

    /// The effective cell resolution into each physical space dimension
    SpaceVec effective_resolution() const override {
        // TODO Implement properly!
        SpaceVec res_eff;
        res_eff.fill(0.);
        return res_eff;
    }

    /// Get shape of the triangular grid
    MultiIndex shape() const override {
        //TODO Implement properly!
        MultiIndexType<Space::dim> shape;
        shape.fill(0);
        return shape;
    }

    /// Structure of the grid
    GridStructure structure() const override {
        return GridStructure::triangular;
    }


    // .. Position-related methods ............................................
    /// Returns the multi index of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    MultiIndex midx_of(const IndexType) const override {
        throw std::runtime_error("The TriangularGrid::midx_of method is not "
                                 "yet implemented!");
        return {};
    }

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    SpaceVec barycenter_of(const IndexType) const override {
        throw std::runtime_error("The TriangularGrid::barycenter_of method "
                                 "is not yet implemented!");
        return {};
    }

    /// Returns the extent of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    SpaceVec extent_of(const IndexType) const override {
        throw std::runtime_error("The TriangularGrid::extent_of method is not "
                                 "yet implemented!");
        return {};
    }

    /// Returns the vertices of the cell with the given ID
    /** \warning The order of the vertices is not guaranteed.
      *
      * \note    This method does not perform bounds checking of the given ID!
      */
    std::vector<SpaceVec> vertices_of(const IndexType) const override {
        throw std::runtime_error("The TriangularGrid::vertices_of method is "
                                 "not yet implemented!");
        return {};
    }

    /// Return the ID of the cell covering the given point in physical space
    /** Cells are interpreted as covering half-open intervals in space, i.e.,
      * including their low-value edges and excluding their high-value edges.
      * The special case of points on high-value edges for non-periodic space
      * behaves such that these points are associated with the cells at the
      * boundary.
      *
      * \note   This function always returns IDs of cells that are inside
      *         physical space. For non-periodic space, a check is performed
      *         whether the given point is inside the physical space
      *         associated with this grid. For periodic space, the given
      *         position is mapped back into the physical space.
      */
    IndexType cell_at(const SpaceVec&) const override {
        throw std::runtime_error("The TriangularGrid::cell_at method is not "
                                 "yet implemented!");
        return {};
    }

    /// Retrieve a set of cell indices that are at a specified boundary
    /** \note   For a periodic space, an empty container is returned; no error
      *         or warning is emitted.
      *
      * \param  select  Which boundary to return the cell IDs of. If 'all',
      *         all boundary cells are returned. Other available values depend
      *         on the dimensionality of the grid:
      *                1D:  left, right
      *                2D:  bottom, top
      *                3D:  back, front
      */
    std::set<IndexType> boundary_cells(std::string={}) const override {
        throw std::runtime_error("The TriangularGrid::boundary_cells method "
                                 "is not yet implemented!");
        return {};
    }


protected:
    // -- Neighborhood interface ----------------------------------------------
    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode, const Config&) override
    {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' neighborhood available for TriangularGrid!");
        }
    }

    /// Computes the expected number of neighbors for a neighborhood mode
    DistType expected_num_neighbors(const NBMode& nb_mode,
                                    const Config&) const override
    {
        if (nb_mode == NBMode::empty) {
            return 0;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' neighborhood available for TriangularGrid!");
        }
    }



    // .. Neighborhood implementations ........................................
    // ...
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_TRIANGULAR_HH
