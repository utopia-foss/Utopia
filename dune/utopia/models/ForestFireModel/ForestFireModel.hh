#ifndef UTOPIA_MODELS_FORESTFIREMODEL_HH
#define UTOPIA_MODELS_FORESTFIREMODEL_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>


namespace Utopia {
namespace Models {
namespace ForestFireModel {

enum State : unsigned short int { empty=0, tree=1, burning=2 };

/// Typehelper to define data types of ForestFireModel model 
using ForestFireModelTypes = ModelTypes<State>;
// NOTE if you do not use the boundary condition type, you can delete the
//      definition of the struct above and the passing to the type helper


/// The ForestFireModel Model
/** Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
template<class ManagerType>
class ForestFireModel:
    public Model<ForestFireModel<ManagerType>, ForestFireModelTypes>
{
public:
    /// The base model type
    using Base = Model<ForestFireModel<ManagerType>, ForestFireModelTypes>;
    
    /// Data type of the state
    using Data = typename Base::Data;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

    /// Data type that holds the configuration
    using Config = typename Base::Config;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    // Alias the neighborhood classes to make access more convenient
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// Model parameters
    const double _growth_rate;
    const double _lightning_frequency;
    const double _resistance;  


    // -- Datasets -- //
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in the constructor.
    std::shared_ptr<DataSet> _dset_state;


    // -- Rule functions -- //
    // states: 0: empty, 1: tree, 2: burning 
    // empty -> tree with probability growth_rate
    // tree -> burning with probability lightning_frequency
    // tree -> burning with probability 1 - resistance if i' in Neighborhood of i burning
    // burning -> empty

    /// Sets the given cell to state "empty"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_empty = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = empty;

        return state;
    };


    /// Sets the given cell to state "tree"
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_tree = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state = tree;

        return state;
    };


    /// Define the update rule for a cell
    std::function<State(std::shared_ptr<CellType>)> _update = [this](const auto cell){

        // Get the state of the cell
        auto state = cell->state();

        std::uniform_real_distribution<> dist(0., 1.);
        // state is empty, empty -> tree by growth
        if (state == empty && dist(*this->_rng) < _growth_rate) {  
            state = tree; 
        }

        // state is tree
        else if (state == tree)
        {
            // tree -> burning by lightning
            if (dist(*this->_rng) < _lightning_frequency) {
                state = burning; 
            }

            // tree -> burning by catching fire from neighbor
            else 
            {
                for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
                {
                    if (nb->state() == burning && dist(*this->_rng) < 1-_resistance) // i' burning
                    {
                        state = burning;
                        break;
                    }
                }
            }
        }

        // state is burning, burning -> empty
        else if (state == burning) {
            state = empty;
        }

        // Return the new state cell
        return state;
    };


public:
    /// Construct the ForestFireModel model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    ForestFireModel (const std::string name,
                 ParentModel &parent,
                 ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _growth_rate(as_double(this->_cfg["growth_rate"])),
        _lightning_frequency(as_double(this->_cfg["lightning_frequency"])),
        _resistance(as_double(this->_cfg["resistance"])),
        // create datasets
        _dset_state(this->_hdfgrp->open_dataset("state"))      
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Set the capacity of the datasets
        // We know the maximum number of steps (== #rows), and the number of
        // grid cells (== #columns); that is the final extend of the dataset.
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_state->set_capacity({this->get_time_max() + 1, num_cells});

        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // Apply a rule to all cells depending on the config value
        if (initial_state == "empty")
        {
            apply_rule(_set_initial_state_empty, _manager.cells());
        }
        else if (initial_state == "tree")
        {
            apply_rule(_set_initial_state_tree, _manager.cells());
        }
        else
        {
            throw std::runtime_error("The initial state is not valid!");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Here you can add a detailed description what exactly happens 
     *          in a single iteration step
     */
    void perform_step ()
    {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_update, _manager.cells());
    }


    /// Write data
    void write_data ()
    {   
        // state
        _dset_state->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state();
                                });
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};

} // namespace ForestFireModel
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_FORESTFIREMODEL_HH
