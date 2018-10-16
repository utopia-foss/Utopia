#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace Utopia{
namespace DataIO{

/// The monitor timer keeps track of the time when to emit monitor data
class MonitorTimer{
public:
    // -- Data types uses throughout the monitor timer-- //

    /// Data type for the clock
    using Clock = std::chrono::high_resolution_clock;

    /// Data type for a time point
    using Time = std::chrono::high_resolution_clock::time_point;

    /// Data type for the time unit
    using TimeUnit = std::chrono::milliseconds;

private:
    // -- Member declaration -- //
    /// The emit interval
    const std::size_t _emit_interval;

    /// The time of the last emit
    Time _last_emit;

public:
    /// Constructor
    /** Construct a new Monitor Timer object. The _last_emit time is set to the
     * time of construction.
     * @param emit_interval The time interval that defines whether the time has
     * come to emit data. If more time than the _emit_interval has passed
     * the time_has_come function returns true.
     */
    MonitorTimer(   const std::size_t emit_interval) :
                    _emit_interval(emit_interval),
                    // Initialize the time of the last commit to current time
                    _last_emit(Clock::now()) {}

    /// Check for whether the time to emit has come or not.
    /**
     * @param reset Reset the internal timer to the current time
     *        if the _emit_interval has been exceeded.
     * @return true if the internal timer has exceeded the _last_emit time.
     */
    bool time_has_come(const bool reset=false){
        // Calculate the time difference between now and the last emit
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<TimeUnit>(now - _last_emit);
        
        // If more time than the _emit_interval has passed return true
        if ((std::size_t)duration.count() > _emit_interval) {
            if (reset){
                _last_emit = now;
            }
            return true;
        }
        else{
            return false;
        }
    }
};


class MonitorData{
private:
    YAML::Node _data;
public:
    MonitorData() : _data(YAML::Node()) {
        // Specify that the emitted data is shown in a single line
        _data.SetStyle(YAML::EmitterStyle::Flow);
    };

    template<typename Value>
    void set_entry(const std::string model_name, const std::string key, const Value value){
        _data[model_name + "." + key] = value;
    }

    void emit(){
        std::cout << _data << std::endl;
    }
};


class MonitorManager{
private:

    using Timer = std::shared_ptr<MonitorTimer>;

    Timer _timer;

    MonitorData _data;

public:
    MonitorManager(const std::size_t emit_interval) :
                    _timer(std::make_shared<MonitorTimer>(emit_interval)),
                    // Create an empty MonitorData object for the data to be emitted
                    _data(MonitorData()) {};

    void perform_emission(){
        if (time_has_come(true)){
            _data.emit();
        }
    }

    bool time_has_come(const bool reset=false){
        return _timer->time_has_come(reset);
    }

    Timer& get_timer(){
        return _timer;
    }

    MonitorData& get_data(){
        return _data;
    }

};

class Monitor{
private:
    const std::string _name;

    std::shared_ptr<MonitorManager> _mtr_mgr;
public:
    Monitor(const std::string name,
            MonitorManager root_mtr):
                _name(name),
                _mtr_mgr(std::make_shared<MonitorManager>(root_mtr)){};

    Monitor(const std::string name,
            Monitor& parent_mtr):
                _name(parent_mtr.get_name() + "." + name),
                _mtr_mgr(parent_mtr.get_mtr_mgr()){};

    template <typename Value>
    void set_entry(const std::string key, const Value value){
        _mtr_mgr->get_data().set_entry(_name, key, value);
    }

    template <typename Value>
    void set_entry_if_time_is_ripe(const std::string key, const Value value){
        if (_mtr_mgr->time_has_come(false)){
            set_entry(key, value);   
        }
    }

    std::shared_ptr<MonitorManager> get_mtr_mgr() const {
        return _mtr_mgr;
    }

    std::string get_name() const {
        return _name;
    }
};

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_MONITOR_HH