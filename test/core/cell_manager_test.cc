#include <cassert>
#include <iostream>
#include <random>

#include <utopia/core/logging.hh>
#include <utopia/data_io/cfg_utils.hh>

#include "testtools.hh"
#include "cell_manager_test.hh"

using namespace Utopia::Test::CellManager;

// Import some types
using Utopia::NBMode;
using Utopia::Update;
using Utopia::get_as;

/// A cell state definition that is default-constructible
struct CellStateDC {
    double a_double = 0.;
    std::string a_string = "";
    bool a_bool = false;
};

/// A cell state definition that is config-constructible
struct CellStateCC {
    double a_double;
    std::string a_string;
    bool a_bool;

    CellStateCC(const Config& cfg)
    :
        a_double(get_as<double>("a_double", cfg)),
        a_string(get_as<std::string>("a_string", cfg)),
        a_bool(get_as<bool>("a_bool", cfg))
    {}
};

/// A cell state definition that is config-constructible and has an RNG
struct CellStateRC {
    double a_double;
    std::string a_string;
    bool a_bool;

    template<class RNG>
    CellStateRC(const Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        a_double(get_as<double>("a_double", cfg)),
        a_string(get_as<std::string>("a_string", cfg)),
        a_bool(get_as<bool>("a_bool", cfg))
    {
        // Do something with the rng
        std::uniform_real_distribution<double> dist(0., a_double);
        a_bool = dist(*rng);
    }
};

/// A cell state definition that is only explicitly constructible
struct CellStateEC {
    double a_double;
    std::string a_string;
    bool a_bool;

    CellStateEC(double d, std::string s, bool b)
    :
        a_double(d),
        a_string(s),
        a_bool(b)
    {}
};


/// A custom links definition
template<typename CellContainerType>
struct TestLinks {
    /// A container of other cells that are "followed" by this cell ...
    CellContainerType following;
};


// Create some cell traits definitions, i.e. bundling the cell traits together
// The second template parameters defines the update mode, the third whether
// the default constructor is to be used.

/// For a default-constructible cell state
using CellTraitsDC = Utopia::CellTraits<CellStateDC, Update::sync, true>;

/// Default constructible, manual update
using CellTraitsDM = Utopia::CellTraits<CellStateDC, Update::manual, true>;

/// For a config-constructible cell state
using CellTraitsCC = Utopia::CellTraits<CellStateCC, Update::sync>;

/// For a config-constructible cell state (with RNG) 
using CellTraitsRC = Utopia::CellTraits<CellStateCC, Update::sync>;

/// For an explicitly-constructible cell state
using CellTraitsEC = Utopia::CellTraits<CellStateEC, Update::sync>;

/// Cell traits with custom links
using CellTraitsCL = Utopia::CellTraits<CellStateDC,
                                        Update::sync,
                                        true,    // use default constructor
                                        Utopia::EmptyTag,
                                        TestLinks>;


// ----------------------------------------------------------------------------

int main() {
    try {
        std::cout << "Getting config file ..." << std::endl;
        auto cfg = YAML::LoadFile("cell_manager_test.yml");
        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing mock model initialization via ... ------"
                  << std::endl;
        
        // Initialize the mock model with default-constructible cell type
        std::cout << "... default-constructible state" << std::endl;
        MockModel<CellTraitsDC> mm_dc("mm_dc", cfg["default"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with default-constructible cell type
        std::cout << "... default-constructible state" << std::endl;
        MockModel<CellTraitsDC> mm_dc_np("mm_dc_np", cfg["default_np"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "... DataIO::Config-constructible state" << std::endl;
        MockModel<CellTraitsCC> mm_cc("mm_cc", cfg["config"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "... DataIO::Config-constructible state (with RNG)"
                  << std::endl;
        MockModel<CellTraitsRC> mm_rc("mm_rc", cfg["config_with_RNG"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "... only explicitly constructible state" << std::endl;
        const auto initial_state = CellStateEC(2.34, "foobar", true);
        MockModel<CellTraitsEC> mm_ec("mm_ec", cfg["explicit"],
                                      initial_state);
        std::cout << "Success." << std::endl << std::endl;


        // TODO Test passing of custom config



        // -------------------------------------------------------------------
        std::cout << "------ Testing grid structures ... ------"
                  << std::endl;

        std::cout << "... square" << std::endl;
        MockModel<CellTraitsDC> mm_dc_sqr("mm_dc_sqr", cfg["default_sqr"]);
        const auto grid_sqr = mm_dc_sqr._cm.grid().get();
        using exp_t_sqr = const Utopia::SquareGrid<Utopia::DefaultSpace>;
        assert(dynamic_cast<exp_t_sqr*>(grid_sqr));
        std::cout << "Success." << std::endl << std::endl;

        std::cout << "... hexagonal" << std::endl;
        MockModel<CellTraitsDC> mm_dc_hex("mm_dc_hex", cfg["default_hex"]);
        const auto grid_hex = mm_dc_hex._cm.grid().get();
        using exp_t_hex = const Utopia::HexagonalGrid<Utopia::DefaultSpace>;
        assert(dynamic_cast<exp_t_hex*>(grid_hex));
        std::cout << "Success." << std::endl << std::endl;
        
        std::cout << "... triangular" << std::endl;
        MockModel<CellTraitsDC> mm_dc_tri("mm_dc_tri", cfg["default_tri"]);
        const auto grid_tri = mm_dc_tri._cm.grid().get();
        using exp_t_tri = const Utopia::TriangularGrid<Utopia::DefaultSpace>;
        assert(dynamic_cast<exp_t_tri*>(grid_tri));
        std::cout << "Success." << std::endl << std::endl;

        


        // -------------------------------------------------------------------
        std::cout << "------ Testing member access ... ------" << std::endl;
        // Check that parameters were passed correctly
        auto cm = mm_ec._cm;  // All mm_* should have the same parameters.

        auto space = cm.space();  // shared pointer
        auto grid = cm.grid();    // shared pointer
        auto cells = cm.cells();  // const reference
        
        assert(space->dim == 2);
        assert(space->periodic == true);
        assert(space->extent[0] == 2.);
        assert(space->extent[1] == 2.);

        assert(grid->shape()[0] == 42 * 2);
        assert(grid->shape()[1] == 42 * 2);

        assert(cells.size() == ((42 * 2) * (42 * 2)));
        assert(cells[0]->state().a_double == 2.34);
        assert(cells[0]->state().a_string == "foobar");
        assert(cells[0]->state().a_bool == true);

        assert(cm.nb_size() == 0);  // no neighborhood set yet

        std::cout << "Success." << std::endl << std::endl;
        


        // -------------------------------------------------------------------
        std::cout << "------ Testing error messages ------" << std::endl;
        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg",
                                              cfg["missing_grid_cfg"],
                                              initial_state);
            }, "Missing config entry 'cell_manager' in model configuration"));

        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg2",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg2",
                                              cfg["missing_grid_cfg2"],
                                              initial_state);
            }, "Missing grid configuration parameter 'resolution'!"));

        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg3",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg3",
                                              cfg["missing_grid_cfg3"],
                                              initial_state);
            }, "Missing required grid configuration entry 'structure'!"));

        assert(check_error_message<std::invalid_argument>(
            "bad_grid_cfg",
            [&](){
                MockModel<CellTraitsEC> mm_ec("bad_grid_cfg",
                                              cfg["bad_grid_cfg"],
                                              initial_state);
            }, "Invalid value for grid 'structure' argument: 'not_a_valid_"));

        assert(check_error_message<std::invalid_argument>(
            "missing_cell_params",
            [&](){
                MockModel<CellTraitsCC> mm_cc("missing_cell_params",
                                              cfg["missing_cell_params"]);
            }, "missing the configuration entry 'cell_params' to set up"));
        
        std::cout << "Success." << std::endl << std::endl;



        // -------------------------------------------------------------------
        std::cout << "------ Testing custom links ... ------"
                  << std::endl;

        { // Local test scope

        // Initialize a model with a custom link container
        MockModel<CellTraitsCL> mm_cl("mm_cl", cfg["default"]);

        // Get cell manager and two cells
        auto cmcl = mm_cl._cm;
        auto c0 = cmcl.cells()[0]; // shared pointer
        auto c1 = cmcl.cells()[1]; // shared pointer

        // Associate them with each other
        c0->custom_links().following.push_back(c1);
        c1->custom_links().following.push_back(c0);
        std::cout << "Linked two cells." << std::endl;

        // Test access
        assert(c0->custom_links().following[0]->id() == 1);
        assert(c1->custom_links().following[0]->id() == 0);
        std::cout << "IDs match." << std::endl;

        std::cout << "Success." << std::endl << std::endl;

        } // End of local test scope


        // -------------------------------------------------------------------
        std::cout << "------ Testing neighborhood choice ... ------"
                  << std::endl;
        // NOTE The actual neighborhood tests are performed separately

        // without any neighborhood configuration
        std::cout << "... empty" << std::endl;
        MockModel<CellTraitsDC> mm_nb_empty("mm_nb_empty",
                                            cfg["nb_empty"]);
        assert(mm_nb_empty._cm.nb_mode() == NBMode::empty);
        std::cout << "Success." << std::endl << std::endl;

        // vonNeumann neighbor
        std::cout << "... vonNeumann" << std::endl;
        MockModel<CellTraitsDC> mm_nb_vonNeumann("mm_nb_vonNeumann",
                                              cfg["nb_vonNeumann"]);
        assert(mm_nb_vonNeumann._cm.nb_mode() == NBMode::vonNeumann);
        assert(mm_nb_vonNeumann._cm.nb_size() == 4);
        std::cout << "Success." << std::endl << std::endl;

        // pre-computing and storing the values
        std::cout << "... vonNeumann (computed and stored)" << std::endl;
        MockModel<CellTraitsDC> mm_nb_computed("mm_nb_computed",
                                               cfg["nb_computed"]);
        assert(mm_nb_computed._cm.nb_mode() == NBMode::vonNeumann);
        assert(mm_nb_computed._cm.nb_size() == 4);
        std::cout << "Success." << std::endl << std::endl;

        // bad neighborhood specification
        std::cout << "... bad neighborhood mode" << std::endl;
        assert(check_error_message<std::invalid_argument>(
            "nb_bad1",
            [&](){
                MockModel<CellTraitsDC> mm_nb_bad1("mm_nb_bad1",
                                                   cfg["nb_bad1"]);
            },
            "Got unexpected neighborhood mode 'bad'! Available modes: "
            "empty, vonNeumann, Moore, hexagonal."));

        assert(check_error_message<std::invalid_argument>(
            "nb_bad2",
            [&](){
                MockModel<CellTraitsDC> mm_nb_bad2("mm_nb_bad2",
                                                   cfg["nb_bad2"]);
            }, "No 'vonNeumann' neighborhood available for TriangularGrid"));
        std::cout << "Success." << std::endl << std::endl;


        // -------------------------------------------------------------------
        std::cout << "------ Testing position-interface ... ------"
                  << std::endl;

        { // Local test scope

        // Use a previously construct mock model's cell manager
        auto cm = mm_dc._cm;
        auto c0 = cm.cells()[0]; // shared pointer

        // Only test callability; function is tested in the grid tests
        cm.midx_of(c0);
        cm.midx_of(*c0);

        cm.barycenter_of(c0);
        cm.barycenter_of(*c0);

        cm.extent_of(c0);
        cm.extent_of(*c0);

        cm.vertices_of(c0);
        cm.vertices_of(*c0);

        assert(cm.grid()->is_periodic());
        cm.cell_at({3.14, 42.0});
        cm.cell_at({-1.23, 3.45});
        
        cm.boundary_cells();
        cm.boundary_cells("all");
        cm.boundary_cells("left");
        cm.boundary_cells("right");
        cm.boundary_cells("top");
        cm.boundary_cells("bottom");

        std::cout << "Success." << std::endl << std::endl;

        } // End of local test scope

        // -------------------------------------------------------------------
        std::cout << "------ Testing selection-interface ... ------"
                  << std::endl;

        { // Local test scope
        using Utopia::SelectionMode;

        // Use a previously construct mock model's cell manager
        auto cm = mm_dc_np._cm; // with non-periodic space

        // ... and do some general tests
        auto c1 = cm.select_cells<SelectionMode::sample>(42);
        auto c2 = cm.select_cells(cfg["select_cell"]);
        assert(c1.size() == 42);
        assert(c2.size() == 42);
        assert(c1 != c2);

        // NOTE The rest of select_cells is tested by select_test.cc

        std::cout << "Success." << std::endl << std::endl;

        } // End of local test scope

        // -------------------------------------------------------------------
        std::cout << "------ Testing cell state setter ... ------"
                  << std::endl;

        { // Local test scope
        // Construct mock model for this test and extract cell manager
        MockModel<CellTraitsDM> mm_scs("mm_scs", cfg["set_cell_state"]);
        auto cm = mm_scs._cm;

        // Check the initial states are all 0. and false
        for (const auto& cell : cm.cells()) {
            assert(cell->state.a_double == 0.);
            assert(cell->state.a_bool == false);
        }

        // Check the grid shape is uneven
        const auto grid_shape = cm.grid()->shape();
        assert(grid_shape[0] == 4);
        assert(grid_shape[1] == 8);

        // Set from hdf5 file
        cm.set_cell_states<double>("cell_manager_test.h5",
                                   "set_cell_state/cell_ids",
            [](auto& cell, const double val){
                cell->state.a_double = val;
            }
        );

        cm.set_cell_states<int>("cell_manager_test.h5",
                                "set_cell_state/ones",
            [](auto& cell, const int val){
                cell->state.a_bool = static_cast<bool>(val);
            }
        );

        // Check the states were carried over
        for (const auto& cell : cm.cells()) {
            assert(cell->state.a_double == cell->id());
            assert(cell->state.a_bool);
        }

        // Check error messages
        assert(check_error_message<std::runtime_error>(
            "failed loading data",
            [&cm]() mutable {
                cm.set_cell_states<int>("cell_manager_test.h5",
                                        "set_cell_state/i_do_not_exist",
                    [](auto&, const int){}
                );
            }, "Failed loading HDF5 data!")
        );
        assert(check_error_message<std::invalid_argument>(
            "shape mismatch",
            [&cm]() mutable {
                cm.set_cell_states<float>("cell_manager_test.h5",
                                          "set_cell_state/bad_shape",
                    [](auto&, const float){}
                );
            }, "Shape mismatch between loaded data (4, 4) and grid (4, 8)!")
        );


        std::cout << "Success." << std::endl << std::endl;

        } // End of local test scope
        

        // -------------------------------------------------------------------
        // Done.
        std::cout << "------ Total success. ------" << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
