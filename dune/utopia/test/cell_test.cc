#include <random>
#include <iostream>
#include <cassert>

#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>

#include "cell_test.hh"

/// Choose random states and traits. Verify Entity members before and after update
int main(int argc, char *argv[])
{
    try{
        Dune::MPIHelper::instance(argc, argv);

        //some typedefinitions
        using State = int;
        using Position = Dune::FieldVector<double,2>;
        using Index = int;

        //create random devices
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<State> dist_int(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
        std::uniform_real_distribution<double> dist_real(std::numeric_limits<double>::min(),std::numeric_limits<double>::max());

        //Create Objects that are handed to the constructor
        const Position pos({dist_real(gen),dist_real(gen)});
        const Index index = dist_int(gen);
        const bool boundary = true;
        const State state(dist_int(gen));
        
        //build a cell
        Utopia::Cell<State,true,Position,Utopia::DefaultTag,Index> c1(state,pos,boundary,index);

        //assert that the content of the cell is correct
        assert(c1.state()==state);
        assert_cell_members(c1, pos, index, boundary);

        //test whether the default flag is set correctly
        //it was created by the default constructor and not set explicitly here
        Utopia::DefaultTag tag;
        assert(c1.is_tagged==tag.is_tagged);
        
        
        // Test async Cell with doubles
        Utopia::Cell<double, false, Position, Utopia::DefaultTag, int> test_cell(0.1,pos,false,0);
        assert(!test_cell.is_sync());
        auto& c_state = test_cell.state();
        c_state = 0.2;
        assert(test_cell.state() = 0.2);
        // Test members inherited from Tag
        assert(!test_cell.is_tagged);
        test_cell.is_tagged = true;
        assert(test_cell.is_tagged);
        // Test entity member variables
        assert(test_cell.id() == 0);

        // Test sync Cell with vector 
        std::vector<double> vec({0.1, 0.2});
        Utopia::Cell<std::vector<double>, true,Position, Utopia::DefaultTag, int> test_cell2(vec,pos,false, 987654321);
        assert(test_cell2.id() == 987654321);
        assert(test_cell2.is_sync());
        auto& new_state = test_cell2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(test_cell2.state() == vec);
        test_cell2.update();
        assert(test_cell2.state()[1] == 0.3);

        Utopia::Cell<std::vector<double>, true, Position, Utopia::DefaultTag, int,1 > cell_with_neighbors(vec,pos,false, 987654321);
        auto& nb=cell_with_neighbors.neighborhoods();
        assert(nb.size()==1);
        assert(nb[0].size()==0);
        
        auto neighbor= std::make_shared<Utopia::Cell<std::vector<double>, true, Position, Utopia::DefaultTag, int,1 > >(vec,pos,false, 987654321);
        nb[0].push_back(neighbor);
        auto nn=cell_with_neighbors.neighborhoods();
        assert(nn[0].size()==1);
        
        
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