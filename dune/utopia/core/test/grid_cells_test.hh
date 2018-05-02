#include <cassert>
#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/data_vtk.hh>

template<typename Grid, typename CellContainer>
void assert_cells_on_grid(std::shared_ptr<Grid> grid, CellContainer& cells)
{
    using GridTypes = Utopia::GridTypeAdaptor<Grid>;
    using Mapper = typename GridTypes::Mapper;
    auto gv = grid->leafGridView();
    Mapper mapper(gv, Dune::mcmgElementLayout());

    for(const auto& e : elements(gv)){
        const auto id = mapper.index(e);

        // check if entity is contained via index
        auto it = std::find_if(cells.cbegin(),cells.cend(),
            [id](const auto c){return c->id() == id;});
        assert(it != cells.cend());

        // check if position is correct
        const auto cell = *it;
        assert(cell->position() == e.geometry().center());

        // check if boundary info is correct
        bool boundary = false;
        for(const auto& is : intersections(gv,e)){
            if(!is.neighbor()){
                boundary = true;
                break;
            }
        }
        assert(cell->is_boundary() == boundary);
    }
}

/// Assure that periodic grid has the correct NextNeighbor count
template<typename Manager>
void check_grid_neighbors_count (const Manager& manager)
{
    const int nb_count = (Manager::Traits::dim == 2 ? 4 : 6);

    bool exception = false;
    for(const auto cell : manager.cells()){
        const auto neighbors = Utopia::Neighborhoods::NextNeighbor::neighbors(cell,manager);
        if(neighbors.size() != nb_count){
            std::cerr << "Cell No. " << cell->id()
                << " has " << neighbors.size()
                << " neighbors!" << std::endl;
            exception = true;
        }
    }
    if(exception){
        DUNE_THROW(Dune::Exception,
                   "Wrong number of neighbors on at least one cell!");
    }
}

/// Mark neighbors of a cell and cell itself for visual testing
template<typename NB, class Cell, class Manager>
void mark_neighbors (const std::shared_ptr<Cell> cell, const Manager &mngr, const int increment=1)
{
    cell->state_new() -= increment;
    cell->update();
    const auto neighbors = NB::neighbors(cell, mngr);
    for (unsigned int i = 0; i<neighbors.size(); ++i) {
        neighbors[i]->state_new() += increment;
        neighbors[i]->update();
    }
}

/// Plot a visual of the neighborhood of a cell
template<typename NB, typename ID, class M1, class M2>
void visual_check (const ID id, const M1 &m1, const M2 &m2, const std::string prefix)
{
    // Mark neighbors of the given cell id
    mark_neighbors<NB>(m1.cells()[id], m1, 1);
    mark_neighbors<NB>(m2.cells()[id], m2, 2);

    // Write out both grids
    auto vtkwriter = Utopia::Output::create_vtk_writer(m1.grid(), prefix);
    vtkwriter->add_adaptor(Utopia::Output::vtk_output_cell_state(m1.cells()));
    vtkwriter->write(0);
}

/// Assure that periodic grid has the correct Neighbor count
template<class NBClass, int nb_count, typename Manager>
constexpr void check_grid_neighbors_count (const Manager& manager)
{
    bool exception = false;

    for(const auto cell : manager.cells()){
        const auto neighbors = NBClass::neighbors(cell, manager);
        if(neighbors.size() != nb_count){
            std::cerr << "Cell No. " << cell->id()
                << " has " << neighbors.size()
                << " neighbors! Expected " << nb_count << std::endl;
            exception = true;
        }
    }

    if(exception){
        DUNE_THROW(Dune::Exception, "Wrong number of neighbors!");
    }
}

/// Compare the neighborhood implementations for two manager types
template<typename NBClass, typename M1, typename M2>
void compare_neighborhoods (const M1& m1, const M2& m2, const std::string comp_case)
{
    // Go over all cells 
    for(std::size_t i=0; i<m1.cells().size(); ++i){
        // Get the neighbors of this particular cell `i`
        const auto nb1 = NBClass::neighbors(m1.cells()[i], m1);
        const auto nb2 = NBClass::neighbors(m2.cells()[i], m2);
        
        // check size and output detailed error information in case of mismatch
        if (nb1.size() != nb2.size()) {
            std::cerr << "Mismatch of neighborhood size for " << comp_case
                      << " and cell with index " << i << ": "
                      << nb1.size() << " != "  << nb2.size() << std::endl;
            
            visual_check<NBClass>(i, m1, m2, comp_case);
            std::cerr << "Visual check output generated." << std::endl;

            std::cerr << "Cell indices in m1 neighborhood:" << std::endl;
            for (auto&& cell : nb1) { std::cerr << " " << cell->id(); }
            std::cerr << std::endl;

            std::cerr << "Cell indices in m2 neighborhood:" << std::endl;
            for (auto&& cell : nb2) { std::cerr << " " << cell->id(); }
            std::cerr << std::endl;

            DUNE_THROW(Dune::Exception, "Mismatch of neighborhood size!");
        }
        
        // check actual neighbors
        for(auto a : nb1){
            assert(std::find_if(nb2.begin(),nb2.end(),
                    [&a](auto b){ return a == b; })
                != nb2.end());
        }
    }
}

/// Assure consistent neighborhood implementations
/** Assure that cells are instantiated correctly and neighborhood 
 *  implementations on structured and unstructured grids mirror each other
 * 
 * \param cells_per_dim Number of cells the grid should have in each dimension
 * 
 * \tparam dim The grid dimension
 */
template<int dim, bool sync>
void cells_on_grid_test (const unsigned int cells_per_dim)
{
    // Alias the neighborhood classes
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

    // Setup grid and cells
    auto grid = Utopia::Setup::create_grid<dim>(cells_per_dim);
    auto cells = Utopia::Setup::create_cells_on_grid<sync>(grid);

    // structured, non-periodic
    auto m1 = Utopia::Setup::create_manager_cells<true,false>(grid,cells);
    // unstructured, non-periodic
    auto m2 = Utopia::Setup::create_manager_cells<false,false>(grid,cells);
    // structured, periodic
    auto m3 = Utopia::Setup::create_manager_cells<true,true>(grid,cells);

    cells.clear(); // ensure that original container is empty

    // assert correct initialization on grid
    assert_cells_on_grid(m1.grid(),m1.cells());
    assert_cells_on_grid(m2.grid(),m2.cells());
    assert_cells_on_grid(m3.grid(),m3.cells());

    // compare neighborhood implementations (m1: structured, m2: unstructured)
    compare_neighborhoods<NextNeighbor>(m1, m2,
                                        std::to_string(dim) + "d_nn");
    compare_neighborhoods<MooreNeighbor>(m1, m2,
                                         std::to_string(dim) + "d_moore");

    // check that neighbor count is correct everywhere on periodic grids
    check_grid_neighbors_count<NextNeighbor, 2*dim>(m3);

    const int moore_nb_count = std::pow(3, dim) - 1;
    check_grid_neighbors_count<MooreNeighbor, moore_nb_count>(m3);

}