#ifndef UTOPIA_CORE_SIGNAL_HH
#define UTOPIA_CORE_SIGNAL_HH

#include <csignal>
#include <atomic>

namespace Utopia {

/// The flag indicating whether to stop whatever is being done right now
/** This needs to be an atomic flag in order to be thread-safe. While the
  * check of this flag is about three times slower than checking a boolean's
  * value (quick-bench.com/IRtD4sQfp_xUwGPa2YrATD4vEyA), this difference is
  * minute to other computations done between two checks.
  */
std::atomic<bool> stop_now;

/// The received signal value
std::atomic<int> received_signum;

/// Default signal handler function, only setting the `stop_now` global flag
void default_signal_handler(int signum) {
    stop_now.store(true);
    received_signum.store(signum);
}

/// Attaches a signal handler for the given signal via sigaction
/** This function constructs a sigaction struct for the given handler and
  * attaches it to the specified signal number.
  *
  * @param  signum  The signal number to attach a custom handler to
  * @param  handler Pointer to the function that should be invoked when the
  *                 specified signal is received.
  */
template<typename Handler>
void attach_signal_handler(int signum, Handler&& handler) {
    // Initialize the signal flag to make sure it is false
    stop_now.store(false);

    // Also initialize the global variable storing the received signal
    received_signum.store(0);

    // Use POSIX-style sigaction definition rather than deprecated signal
    struct sigaction sa;
    sa.sa_handler = handler;
    sa.sa_flags = 0;

    // Add only the given signal to the set of actions
    // See sigaddset documentation: https://linux.die.net/man/3/sigfillset
    sigaddset(&sa.sa_mask, signum);

    sigaction(signum, &sa, NULL);
}

/// Attaches the default signal handler for the given signal
void attach_signal_handler(int signum) {
    attach_signal_handler(signum, default_signal_handler);
}

} // namespace Utopia

#endif // UTOPIA_CORE_SIGNAL_HH
