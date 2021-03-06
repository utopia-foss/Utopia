#ifndef UTOPIA_MODELS_DUMMY_HH
#define UTOPIA_MODELS_DUMMY_HH

#include <utopia/core/model.hh>


namespace Utopia {
namespace Models {
namespace Dummy {

/// Define data types of dummy model
using DummyTypes = ModelTypes<>;

/// Dummy model with simple update rule
/** Holds a vector of doubles and increments its entries by random numbers
 *  with the bounds determined by the boundary condition vector.
 */
class Dummy : public Model<Dummy, DummyTypes>
{
public:
    /// The base model class
    using Base = Model<Dummy, DummyTypes>;

    /// The data type to use for _state and _bc members
    using Data = std::vector<double>;

    // Type shortcut for dataset
    using DataSet = Base::DataSet;

private:
    /// The current state of the model
    Data _state;

    /// The boundary conditions of the model
    Data _bc;

    /// Dataset to write the state to
    std::shared_ptr<DataSet> _dset_state;

public:
    /// Construct the dummy model with an initial state
    /** \param name          Name of this model instance
     *  \param parent_model  The parent model instance this instance resides in
     *  \param initial_state Initial state of the model
     *  \param custom_cfg    A custom configuration to use instead of the
     *                       one extracted from the parent model using the
     *                       instance name
     */
    template <class ParentModel>
    Dummy (
        const std::string& name,
        const ParentModel& parent_model,
        const Data& initial_state,
        const DataIO::Config& custom_cfg = {}
    )
    :
        // Use the base constructor for the main parts
        Base(name, parent_model, custom_cfg),

        // Initialise state and boundary condition members
        _state(initial_state),
        _bc(_state.size(), 1.0),
        _dset_state(this->create_dset("state", {_state.size()}))
    {}


    /// Iterate by one time step
    /** @details This writes random numbers into the state vector, incrementing
     *          the already existing ones. Thus, with numbers between 0 and 1,
     *          the mean value of the state increases by 0.5 for each
     *          performed step.
     */
    void perform_step()
    {
        // Write some random numbers into the state vector
        auto gen = std::bind(std::uniform_real_distribution<>(), *this->_rng);
        std::generate(_bc.begin(), _bc.end(), gen);
        std::transform(_state.begin(), _state.end(),
                       _bc.begin(), _state.begin(),
                       [](const auto a, const auto b) { return a + b; });
    }


    /// Monitor model information
    /** @details Supply a function that calculates the state mean, if the
     *          monitor will perform an emission.
     */
    void monitor ()
    {
        // Supply the state mean to the monitor
        _monitor.set_by_func("state_mean", [this](){
            const double sum = std::accumulate(this->_state.begin(),
                                               this->_state.end(),
                                               0);
            return sum / this->_state.size();
        });
    }


    /// Write data into a dataset that corresponds to the current step
    void write_data()
    {
        _dset_state->write(_state.begin(), _state.end(),
                           [](auto& value) { return value; });
    }


    // -- Getters and Setters -- //

};

} // namespace Dummy
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_DUMMY_HH
