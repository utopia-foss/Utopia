#include <iostream>

#include "ForestFireModel.hh"

using namespace Utopia::Models::ForestFireModel;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        // distinction of periodic / non-periodic grid and two / three state FFM
        if (Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["periodic"]) 
            && Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["two_state_FFM"])) 
        {
            // Periodic grid and async update
            ForestFireModel model("ForestFireModel", pp,
                /* template arguments: State, periodicity=true, 
                 * dimensionality=2, structured grid=true, synchronous update=false
                 */ 
                create_grid_manager_cells<State,true,2,true,false>("ForestFireModel", pp)
            );
            model.run();
        }
        else if (Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["periodic"]) 
            && !Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["two_state_FFM"])) 
        {
                /* template arguments: State, periodicity=true, 
                 * dimensionality=2, structured grid=true, synchronous update=true
                 */ 
            ForestFireModel model("ForestFireModel", pp,
                create_grid_manager_cells<State,true,2,true,true>("ForestFireModel", pp)
            );
            model.run();
        }
        else if (!Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["periodic"]) 
            && Utopia::as_bool(pp.get_cfg()["ForestFireModel"]["two_state_FFM"])) 
        {
                /* template arguments: State, periodicity=false, 
                 * dimensionality=2, structured grid=true, synchronous update=false
                 */ 
            ForestFireModel model("ForestFireModel", pp,
                create_grid_manager_cells<State,false,2,true,false>("ForestFireModel", pp)
            );
            model.run();
        }
        else 
        {
                /* template arguments: State, periodicity=false, 
                 * dimensionality=2, structured grid=true, synchronous update=true
                 */ 
            ForestFireModel model("ForestFireModel", pp,
                create_grid_manager_cells<State,false,2,true,true>("ForestFireModel", pp)
            );
            model.run();
        }

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
