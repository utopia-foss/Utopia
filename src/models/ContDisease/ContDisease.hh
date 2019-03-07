#ifndef UTOPIA_MODELS_CONTDISEASE_HH
#define UTOPIA_MODELS_CONTDISEASE_HH

#include <functional>
#include <random>

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace ContDisease {


// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The values a cell's state can take: empty, tree, infected, source, stone
enum CDCellState : unsigned short {
    /// Unoccupied
    empty = 0,
    /// Cell represents a tree
    tree = 1,
    /// Cell is infected
    infected = 2,
    /// Cell is an infection source: constantly infected, spreading infection
    source = 3,
    /// Cell cannot be infected
    stone = 4
};

/// The full cell struct for the ContDisease model
struct CDCell {
    /// The cell state
    CDCellState state;

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    CDCell (const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        state(empty)
    {
        // Check if initial_density is available to set up cell state
        if (cfg["initial_density"]) {
            const auto rho = get_as<double>("initial_density", cfg);

            if (rho < 0. or rho > 1.) {
                throw std::invalid_argument("initial_density needs to be in "
                    "interval [0., 1.], but was not!");
            }

            // With this probability, the cell state is a tree
            if (std::uniform_real_distribution<double>(0., 1.)(*rng) < rho) {
                state = tree;
            }
        }
    }
};

/// ContDisease model parameter struct
struct Param {

    /// Probability per site and time step to transition from state empty to tree
    const double growth_rate;

    /// Probablity per site and time step for a tree cell to become infected
    /// if an infected cell is in the neighborhood.
    const double p_infect;

    /// Probaility per site and time step for a random point infection of a
    // tree cell
    const double p_rd_infect;

    /// Infection source - set true to activate a constant row of infected
    /// cells at the bottom boundary
    const bool infection_source;

    // Extract if stones are activated
    const bool stones;

    // Extract how stones are to be initialized
    const std::string stone_init;

    // Extract stone density for stone_init = random
    const double stone_density;

    // Extract clustering weight for stone_init = random
    const double stone_cluster;

    /// Construct the parameters from the given configuration node
    Param(const DataIO::Config& cfg)
    :
        growth_rate(get_as<double>("growth_rate", cfg)),
        p_infect(get_as<double>("p_infect", cfg)),
        p_rd_infect(get_as<double>("p_rd_infect", cfg)),
        infection_source(get_as<bool>("infection_source", cfg)),
        stones(get_as<bool>("stones", cfg)),
        stone_init(get_as<std::string>("stone_init", cfg)),
        stone_density(get_as<double>("stone_density", cfg)),
        stone_cluster(get_as<double>("stone_cluster", cfg))
    {
        if ((growth_rate > 1) or (growth_rate < 0)) {
            throw std::invalid_argument("Invalid growth_rate; need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a tree. Was: "
                + std::to_string(growth_rate));
        }
        if ((p_infect > 1) or (p_infect < 0)) {
            throw std::invalid_argument("Invalid p_infect! Need be in range "
                "[0, 1], was " + std::to_string(p_infect));
        }
        if ((p_rd_infect > 1) or (p_rd_infect < 0)) {
            throw std::invalid_argument("Invalid p_rd_infect; Need be a value "
                "in range [0, 1], was " + std::to_string(p_rd_infect));
        }
        if ((stone_density > 1) or (stone_density < 0)) {
            throw std::invalid_argument("Invalid stone_density! Need be a "
                "value in range [0, 1], was " + std::to_string(stone_density));
        }
        if ((stone_cluster > 1) or (stone_cluster < 0)) {
            throw std::invalid_argument("Invalid stone_cluster! Need be a "
                "value in range [0, 1], was " + std::to_string(stone_cluster));
        }
    }
};


// Specialize the CellTraits type helper for this model
/** \detail Specifies the type of each cells' state as first template argument
  *         and the update mode as second.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CDCellTraits = Utopia::CellTraits<CDCell, UpdateMode::sync>;


/// Typehelper to define data types of ContDisease model
using CDModelTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Contagious disease model on a grid
/** \detail In this model, we model the spread of a disease through a forest on
 *          a 2D grid. Each cell can have one of five different states: empty,
 *          tree, infected, source or empty.
 *          Each time step, cells update their state according to the update
 *          rules. Empty cells will convert with a certain probability
 *          to tress, while trees represent cells that can be infected.
 *          Infection can happen either through a neighboring cells, or
 *          through random point infection. An infected cells reverts back to
 *          empty after one time step.
 *          Stones represent cells that can not be infected, therefore
 *          represent a blockade for the spread of the infection.
 *          Infection sources are cells that continuously spread infection
 *          without dying themselves.
 *          Different starting conditions, and update mechanisms can be
 *          configured.
 */
class ContDisease:
    public Model<ContDisease, CDModelTypes>
{
public:
    /// The base model type
    using Base = Model<ContDisease, CDModelTypes>;

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CDCellManager = Utopia::CellManager<CDCellTraits, ContDisease>;

    /// Rule function type
    using RuleFunc = typename CDCellManager::RuleFunc;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CDCellManager _cm;

    /// Model parameters
    const Param _param;

    /// The range [0, 1] distribution to use for probability checks
    std::uniform_real_distribution<double> _prob_distr;

    // .. Temporary objects ...................................................
    /// Densities for all states
    /** \note   This array is used for temporary storage; it is not
      *         automatically updated!
      * \detail The array entries map to the CellState enum:
      *         0: empty
      *         1: tree
      *         2: infected
      *         3: source
      *         4: stone
      */
    std::array<double, 5> _densities;

    // .. Data groups .........................................................
    /// The data group where all density datasets are stored in
    std::shared_ptr<DataGroup> _dgrp_densities;

    // .. Datasets ............................................................
    /// 2D dataset (cell ID and time) of cell states
    std::shared_ptr<DataSet> _dset_state;

    /// 1D dataset of density of empty cells over time
    std::shared_ptr<DataSet> _dset_density_empty;

    /// 1D dataset of density of tree cells over time
    std::shared_ptr<DataSet> _dset_density_tree;

    /// 1D dataset of density of infected cells over time
    std::shared_ptr<DataSet> _dset_density_infected;

    /// 1D dataset of density of infected infection source cells over time
    std::shared_ptr<DataSet> _dset_density_source;

    /// 1D dataset of density of infected stone cells over time
    std::shared_ptr<DataSet> _dset_density_stone;


public:
    /// Construct the ContDisease model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    ContDisease (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Carry over Parameters
        _param(this->_cfg),

        // Initialize remaining members
        _prob_distr(0., 1.),
        _densities{},  // undefined here, will be set in constructor body

        // Create a data group for the densities
        _dgrp_densities(this->_hdfgrp->open_group("densities")),

        // Create dataset for cell states
        _dset_state(this->create_cm_dset("state", _cm)),

        // Create datasets for all densities
        _dset_density_empty(   this->create_dset("empty",
                                                 _dgrp_densities, {})),
        _dset_density_tree(    this->create_dset("tree",
                                                 _dgrp_densities, {})),
        _dset_density_infected(this->create_dset("infected",
                                                 _dgrp_densities, {})),
        _dset_density_source(  this->create_dset("source",
                                                 _dgrp_densities, {})),
        _dset_density_stone(   this->create_dset("stone",
                                                 _dgrp_densities, {}))
    {
        // Make sure the densities are not undefined
        _densities.fill(std::numeric_limits<double>::quiet_NaN());

        // Remaining initialization steps regard macroscopic quantities
        
        // Infection source
        if (_param.infection_source) {
            this->_log->debug("Setting bottom boundary cells to be "
                              "permanently infected ...");
            apply_rule([](const auto& cell) {
                    auto state = cell->state();
                    state.state = source;
                    return state;
                }, _cm.boundary_cells("bottom"));
        }

        // Stones
        if (_param.stones) {
            if (_param.stone_init == "random") {
                this->_log->debug("Setting up random cells to be stones ...");

                // Get a copy of the cells container and shuffle it
                auto cells_shuffled = _cm.cells();
                std::shuffle(cells_shuffled.begin(), cells_shuffled.end(),
                             *this->_rng);

                // Make some parameters available for lambda captures
                const double stone_cluster = _param.stone_cluster;
                const double stone_density = _param.stone_density;

                /// Initialize stones randomly with probability stone_density
                apply_rule([this, &stone_density](const auto& cell) {
                    // Cell will be a stone with probability stone_density
                    auto state = cell->state();
                    if (this->_prob_distr(*this->_rng) < stone_density){
                        state.state = stone;
                        return state;
                    }
                    // else: stay in the same state
                    return state;
                }, cells_shuffled);

                // Add a stone with probability stone_cluster to any empty
                // cell with a neighboring stone.
                apply_rule([this, &stone_cluster](const auto& cell) {
                    auto state = cell->state();

                    // Add the clustered stones
                    // Iterate over all neighbors of the current cell
                    for (auto& nb: this->_cm.neighbors_of(cell)) {
                        auto nb_state = nb->state();

                        if (    state.state == empty
                            and nb_state.state == stone
                            and this->_prob_distr(*this->_rng) < stone_cluster)
                        {
                            // Become a stone
                            state.state = stone;
                        }
                        else {
                            break;
                        }
                    }
                    return state;
                }, cells_shuffled);
            }
        } // end of stones setup

        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);

        // -- Write initial state
        // Write all other data that is written each write_data call, which
        // includes the remaining densities (indices 0, 1, and 2)
        this->write_data();

        // Now that all densities have been calculated (in write_data), write
        // those that do not change throughout the simulation (indices 3 and 4)
        _dset_density_stone->write(_densities[3]);   // stone
        _dset_density_source->write(_densities[4]);  // infection source
        
        this->_log->debug("Initial state written.");
    }

protected:
    // .. Helper functions ....................................................
    /// Update the densities array
    /** @detail   Each density is calculated by counting the number of state
     *            occurrences and afterwards dividing by the total number of
     *            cells.
     * @attention It is possible that rounding errors occur due to the
     *            division, thus, it is not guaranteed that the densities
     *            exactly add up to 1. The errors should be negligible.
     */
    void update_densities() {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state. Use the _densities
        // member for that in order to not create a new array.
        for (const auto& cell : this->_cm.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<unsigned short int>(cell->state().state)];
        }
        // The _densities array now contains the counts.

        const double num_cells = this->_cm.cells().size();

        // Calculate the actual densities by dividing the counts by the total
        // number of cells.
        for (auto&& d : _densities){
            d /= num_cells;
        }
    };


    // .. Rule functions ......................................................

    /// Define the update rule
    /** \detail Update the given cell according to the following rules:
      *         - Empty cells grow trees with probability growth_rate.
      *         - Tree cells in neighborhood of an infected cell get infected
      *           with the probability p_infect.
      *         - Infected cells die and become an empty cell.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell
        auto state = cell->state();

        // Distinguish by current state
        if (state.state == empty) {
            // With a probability of growth_rate, set the cell's state to tree
            if (_prob_distr(*this->_rng) < _param.growth_rate){
                state.state = tree;
                return state;
            }
        }
        else if (state.state == tree){
            // Tree can be infected by neighbor our by random-point-infection.

            // Determine whether there will be a point infection
            if (_prob_distr(*this->_rng) < _param.p_rd_infect) {
                // Yes, point infection occurred.
                state.state = infected;
                return state;
            }
            else {
                // Go through neighbor cells (according to Neighborhood type),
                // and check if they are infected (or an infection source).
                // If yes, infect cell with the probability p_infect.
                for (auto& nb: this->_cm.neighbors_of(cell)) {
                    // Get the neighbor cell's state
                    auto nb_state = nb->state();

                    if (   nb_state.state == infected
                        or nb_state.state == source)
                    {
                        // With a certain probability, become infected
                        if (_prob_distr(*this->_rng) < _param.p_infect) {
                            state.state = infected;
                            return state;
                        }
                    }
                }
            }
        }
        else if (state.state == infected) {
            // Decease -> become an empty cell
            state.state = empty;
            return state;
        }
        // else: other cell states need no update

        // Return the (potentially changed) cell state for the next round
        return state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single time step
    /** \detail This updates all cells (synchronously) according to the
      *         _update rule. For specifics, see there.
      */
    void perform_step () {
        // Apply the update rule to all cells.
        apply_rule(_update, _cm.cells());
        // NOTE The cell state is updated synchronously, i.e.: only after all
        //      cells have been visited and know their state for the next step
    }


    /// Monitor model information
    /** \detail Supplies the `densities` array to the monitor.
      */
    void monitor () {
        update_densities();
        this->_monitor.set_entry("densities", _densities);
    }


    /// Write data
    /** \detail Writes out the cell state and the densities of cells with the
      *         states empty, tree, or infected (i.e.: those that may change)
      */
    void write_data () {
        // Write the cell state
        _dset_state->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state().state);
            }
        );

        // And those densities that are changing (empty, tree, infected)
        update_densities();
        _dset_density_empty->write(_densities[0]);
        _dset_density_tree->write(_densities[1]);
        _dset_density_infected->write(_densities[2]);
    }

};


} // namespace ContDisease
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_CONTDISEASE_HH
