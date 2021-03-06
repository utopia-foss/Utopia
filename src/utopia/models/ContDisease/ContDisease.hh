#ifndef UTOPIA_MODELS_CONTDISEASE_HH
#define UTOPIA_MODELS_CONTDISEASE_HH

#include <functional>
#include <random>

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>
#include <utopia/core/select.hh>

// ContDisease-realted includes
#include "params.hh"
#include "state.hh"


namespace Utopia::Models::ContDisease {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Specialize the CellTraits type helper for this model
/** Specifies the type of each cells' state as first template argument
  * and the update mode as second.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CDCellTraits = Utopia::CellTraits<State, Update::manual>;


/// Typehelper to define data types of ContDisease model
using CDTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Contagious disease model on a grid
/** In this model, we model the spread of a disease through a forest on
 *  a 2D grid. Each cell can have one of five different states: empty, tree, 
 *  infected, source or empty. Each time step, cells update their state 
 *  according to the update rules. Empty cells will convert with a certain 
 *  probability to tress, while trees represent cells that can be infected.
 *  Infection can happen either through a neighboring cells, or through random 
 *  point infection. An infected cells reverts back to empty after one time 
 *  step. Stones represent cells that can not be infected, therefore
 *  represent a blockade for the spread of the infection. Infection sources are 
 *  cells that continuously spread infection without dying themselves.
 *  Different starting conditions, and update mechanisms can be configured.
 */
class ContDisease:
    public Model<ContDisease, CDTypes>
{
public:
    /// The base model type
    using Base = Model<ContDisease, CDTypes>;

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CDCellTraits, ContDisease>;

    /// Type of a cell
    using Cell = typename CellManager::Cell;

    /// Type of a container of shared pointers to cells
    using CellContainer = Utopia::CellContainer<Cell>;

    /// Rule function type
    using RuleFunc = typename CellManager::RuleFunc;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    /// Model parameters
    const Params _params;

    /// The range [0, 1] distribution to use for probability checks
    std::uniform_real_distribution<double> _prob_distr;

    /// The incremental cluster tag
    unsigned int _cluster_id_cnt;

    /// A temporary container for use in cluster identification
    std::vector<std::shared_ptr<CellManager::Cell>> _cluster_members;

    /// Densities for all states
    /** Array indices are linked to \ref Utopia::Models::ContDisease::Kind
      *
      * \warning This array is used for temporary storage; it is not
      *          automatically updated but only upon write operations.
      */
    std::array<double, 5> _densities;

    // .. Data-Output related members .........................................
    bool _write_only_densities;

    /// 2D dataset (densities array and time) of density values
    std::shared_ptr<DataSet> _dset_densities;

    /// 2D dataset (cell ID and time) of cell kinds
    std::shared_ptr<DataSet> _dset_kind;

    /// 2D dataset (tree age and time) of cells
    std::shared_ptr<DataSet> _dset_age;

    /// The dataset for storing the cluster ID associated with each cell
    std::shared_ptr<DataSet> _dset_cluster_id;


public:
    /// Construct the ContDisease model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template<class ParentModel>
    ContDisease (
        const std::string& name,
        ParentModel& parent_model,
        const DataIO::Config& custom_cfg = {}
    )
    :
        // Initialize first via base model
        Base(name, parent_model, custom_cfg),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Carry over Parameters
        _params(this->_cfg),

        // Initialize remaining members
        _prob_distr(0., 1.),
        _cluster_id_cnt(),
        _cluster_members(),
        _densities{},  // undefined here, will be set in constructor body
        _write_only_densities(get_as<bool>("write_only_densities",
                                           this->_cfg)),

        // Create the dataset for the densities; shape is known
        _dset_densities(this->create_dset("densities", {5})),

        // Create dataset for cell states
        _dset_kind(this->create_cm_dset("kind", _cm)),

        // Create dataset for tree age
        _dset_age(this->create_cm_dset("age", _cm)),

        // Create dataset for cluster id
        _dset_cluster_id(this->create_cm_dset("cluster_id", _cm))
    {
        // Make sure the densities are not undefined
        _densities.fill(std::numeric_limits<double>::quiet_NaN());

        // Cells are already set up by the CellManager.
        // Remaining initialization steps regard only macroscopic quantities,
        // e.g. the setup of heterogeneities: Stones and infection source.

        // Stones
        if (_cfg["stones"] and get_as<bool>("enabled", _cfg["stones"])) {
            this->_log->info("Setting cells to be stones ...");

            // Get the container
            auto to_turn_to_stone = _cm.select_cells(_cfg["stones"]);

            // Apply a rule to all cells of that container: turn to stone
            apply_rule<Update::async, Shuffle::off>(
                [](const auto& cell){
                    auto& state = cell->state;
                    state.kind = Kind::stone;
                    return state;
                },
                to_turn_to_stone
            );

            this->_log->info("Set {} cells to be stones using selection mode "
                             "'{}'.", to_turn_to_stone.size(),
                             get_as<std::string>("mode", _cfg["stones"]));
        }
        
        // Ignite some cells permanently: fire sources
        if (    _cfg["infection_source"]
            and get_as<bool>("enabled", _cfg["infection_source"]))
        {
            this->_log->info("Setting cells to be infection sources ...");
            auto source_cells = _cm.select_cells(_cfg["infection_source"]);

            apply_rule<Update::async, Shuffle::off>(
                [](const auto& cell){
                    auto& state = cell->state;
                    state.kind = Kind::source;
                    return state;
                },
                source_cells
            );

            this->_log->info("Set {} cells to be infection sources using "
                "selection mode '{}'.", source_cells.size(),
                get_as<std::string>("mode", _cfg["infection_source"]));
        }

        // Add attributes to density dataset that provide coordinates
        _dset_densities->add_attribute("dim_name__1", "kind");
        _dset_densities->add_attribute("coords_mode__kind", "values");
        _dset_densities->add_attribute("coords__kind",
            std::vector<std::string>{
                "empty", "tree", "infected", "source", "stone"
            });
        this->_log->debug("Added coordinates to densities dataset.");

        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);
    }

protected:
    // .. Helper functions ....................................................
    /// Update the densities array
    /** Each density is calculated by counting the number of state
     *  occurrences and afterwards dividing by the total number of cells.
     * 
     *  @attention It is possible that rounding errors occur due to the
     *             division, thus, it is not guaranteed that the densities
     *             exactly add up to 1. The errors should be negligible.
     */
    void update_densities() {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state. Use the _densities
        // member for that in order to not create a new array.
        for (const auto& cell : this->_cm.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<char>(cell->state.kind)];
        }
        // The _densities array now contains the counts.

        // Calculate the actual densities by dividing the counts by the total
        // number of cells.
        for (auto&& d : _densities){
            d /= static_cast<double>(this->_cm.cells().size());
        }
    };


    /// Identify clusters
    /** This function identifies clusters and updates the cell
     *  specific cluster_id as well as the member variable 
     *  cluster_id_cnt that counts the number of ids
     */
    void identify_clusters(){
        // reset cluster counter
        _cluster_id_cnt = 0;
        apply_rule<Update::async, Shuffle::off>(_identify_cluster, 
                                                _cm.cells());
    }

    /// Apply infection control
    /** Add infections if the iteration step matches the ones specified in 
     *  the configuration. There are two available modes of infection control 
     *  that are applied, if provided, in this order:
     *
     *  1.  At specified times (parameter: ``at_times``) a number of additional
     *      infections is added (parameter: ``num_additional_infections``)
     *  2.  The parameter ``p_infect`` is changed to a new value at given
     *      times. This can happen multiple times.
     *      Parameter: ``change_p_infect``
     */
    void infection_control(){
        // Check that time matches the first element of the sorted queue of 
        // time steps at which to apply the given number of infections.
        if (not _params.infection_control.at_times.empty()){
            // Check whether time has come for infections
            if (this->_time == _params.infection_control.at_times.front()){
                // Select cells that are trees 
                // (not empty, stones, infected, or source)
                const auto cells_pool =
                    _cm.select_cells<SelectionMode::condition>(
                        [&](const auto& cell){
                            return (cell->state.kind == Kind::tree);
                        }
                    );

                // Sample cells from the pool and ...
                CellContainer sample{};
                std::sample(
                    cells_pool.begin(), cells_pool.end(),
                    std::back_inserter(sample),
                    _params.infection_control.num_additional_infections,
                    *this->_rng
                );
                
                // ... and infect the sampled cells
                for (const auto& cell : sample){
                    cell->state.kind = Kind::infected;
                }

                // Done. Can now remove first element of the queue.
                _params.infection_control.at_times.pop();
            }
        }

        // Change p_infect if the iteration step matches the ones
        // specified in the configuration. This leads to constant time lookup.
        if (not _params.infection_control.change_p_infect.empty()) {
            const auto change_p_infect =
                _params.infection_control.change_p_infect.front();

            if (this->_time == change_p_infect.first) {
                _params.p_infect = change_p_infect.second;

                // Done. Can now remove the element from the queue.
                _params.infection_control.change_p_infect.pop();
            }
        }
    }

    // .. Rule functions ......................................................

    /// Define the update rule
    /** Update the given cell according to the following rules:
      * - Empty cells grow trees with probability p_growth.
      * - Tree cells in neighborhood of an infected cell do not get infected
      *   with the probability p_immunity.
      * - Infected cells die and become an empty cell.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell and reset its cluster ID
        auto state = cell->state;
        state.cluster_id = 0;

        // Distinguish by current state
        if (state.kind == Kind::empty) {
            // With a probability of p_growth, set the cell's state to tree
            if (_prob_distr(*this->_rng) < _params.p_growth){
                state.kind = Kind::tree;
                return state;
            }
        }
        else if (state.kind == Kind::tree){
            // Increase the age of the tree
            ++state.age;

            // Tree can be infected by neighbor or by random-point-infection.

            // Determine whether there will be a point infection
            if (_prob_distr(*this->_rng) < _params.p_infect) {
                // Yes, point infection occurred.
                state.kind = Kind::infected;
                return state;
            }
            else {
                // Go through neighbor cells (according to Neighborhood type),
                // and check if they are infected (or an infection source).
                // If yes, infect cell with the probability 1-p_immunity.
                for (const auto& nb: this->_cm.neighbors_of(cell)) {
                    // Get the neighbor cell's state
                    auto nb_state = nb->state;

                    if (   nb_state.kind == Kind::infected
                        or nb_state.kind == Kind::source)
                    {
                        // With a certain probability, become infected
                        if (_prob_distr(*this->_rng) > _params.p_immunity) {
                            state.kind = Kind::infected;
                            return state;
                        }
                    }
                }
            }
        }
        else if (state.kind == Kind::infected) {
            // Decease -> become an empty cell
            state.kind = Kind::empty;

            // Reset the age of the cell to 0
            state.age = 0;

            return state;
        }
        // else: other cell states need no update

        // Return the (potentially changed) cell state for the next round
        return state;
    };

    /// Identify each cluster of trees
    RuleFunc _identify_cluster = [this](const auto& cell){
        if (cell->state.cluster_id != 0 or cell->state.kind != Kind::tree) {
            // already labelled, nothing to do. Return current state
            return cell->state;
        }
        // else: need to label this cell

        // Increment the cluster ID counter and label the given cell
        _cluster_id_cnt++;
        cell->state.cluster_id = _cluster_id_cnt;

        // Use existing cluster member container, clear it, add current cell
        auto& cluster = _cluster_members;
        cluster.clear();
        cluster.push_back(cell);

        // Perform the percolation
        for (unsigned int i = 0; i < cluster.size(); ++i) {
            // Iterate over all potential cluster members c, i.e. all
            // neighbors of cell cluster[i] that is already in the cluster
            for (const auto& nb : this->_cm.neighbors_of(cluster[i])) {
                // If it is a tree that is not yet in the cluster, add it.
                if (    nb->state.cluster_id == 0
                    and nb->state.kind == Kind::tree)
                {
                    nb->state.cluster_id = _cluster_id_cnt;
                    cluster.push_back(nb);
                    // This extends the outer for-loop...
                }
            }
        }

        return cell->state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single time step
    /** This updates all cells (synchronously) according to the _update rule. 
      * For specifics, see there.
      *
      * If infection control is activated, the cells are first modified 
      * according to the specific infection control parameters.
      */
    void perform_step () {
        // Apply infection control if enabled
        if (_params.infection_control.enabled){
            infection_control();
        }

        // Apply the update rule to all cells.
        apply_rule<Update::sync>(_update, _cm.cells());
        // NOTE The cell state is updated synchronously, i.e.: only after all
        //      cells have been visited and know their state for the next step
    }


    /// Monitor model information
    /** Supplies the `densities` array to the monitor.
      */
    void monitor () {
        update_densities();
        this->_monitor.set_entry("densities", _densities);
    }


    /// Write data
    /** Writes out the cell state and the densities of cells with the states 
      * empty, tree, or infected (i.e.: those that may change)
      */
    void write_data () {
        // Update densities and write them
        update_densities();
        _dset_densities->write(_densities);
        // NOTE Although stones and source density are not changing, they are
        //      calculated anyway and writing them again is not a big cost
        //      (two doubles) while making analysis much easier.

        // If only the densities are to be written, can stop here.
        if (_write_only_densities) {
            return;
        }

        // Write the cell state
        _dset_kind->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<char>(cell->state.kind);
            }
        );

        // Write the tree ages
        _dset_age->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state.age);
            }
        );

        // Identify clusters and write them out
        identify_clusters();
        _dset_cluster_id->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state.cluster_id;
        });
    }
};


} // namespace Utopia::Models::ContDisease

#endif // UTOPIA_MODELS_CONTDISEASE_HH
