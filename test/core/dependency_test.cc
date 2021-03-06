// Just include dependencies ...
#include <armadillo>
#include <boost/graph/adjacency_list.hpp>
#include <hdf5.h>
#include <yaml-cpp/yaml.h>

#include "utopia/core/logging.hh"

class GraphTest {
public:
    friend class boost::adjacency_list<>;
};

void armadillo_test () {
    // define two objects
    arma::mat A = arma::randu(3, 3);
    arma::vec x = arma::randu(3);

    // multiply and print result
    auto b = A*x;
    b.print("result:");
}

int main () {
    Utopia::setup_loggers();
    YAML::Node config;
    armadillo_test();
    return 0;
}
