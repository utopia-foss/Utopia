#include <cassert>

#include "grid_cells_test.hh"

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        cells_on_grid_test<3,true>(10);

        return 0;
    }
    catch(Dune::Exception c){
        std::cerr << c << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}