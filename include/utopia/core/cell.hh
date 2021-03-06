#ifndef UTOPIA_CORE_CELL_HH
#define UTOPIA_CORE_CELL_HH

#include "tags.hh"
#include "types.hh"
#include "entity.hh"


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// CellTraits are just another name for Utopia::EntityTraits
template<typename StateType,
         Update update_mode,
         bool use_def_state_constr=false,
         typename CellTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using CellTraits = EntityTraits<StateType,
                                update_mode,
                                use_def_state_constr,
                                CellTags,
                                CustomLinkContainers>;

/// A cell is a slightly specialized state container
/** It can be extended with the use of tags and can be associated with
  * so-called "custom links". These specializations are carried into the cell
  * by means of the CellTraits struct.
  * A cell is embedded into the CellManager, where the discretization allows
  * assigning a position in space to the cell.
  * The cell itself does not need to know anything about that ...
  *
  * \tparam Traits  Valid Utopia::EntityTraits, describing the type of cell
  */
template<typename Traits>
class Cell :
    public Entity<Cell<Traits>, Traits>
{
public:
    /// The type of this cell
    using Self = Cell<Traits>;

    /// The type of the state
    using State = typename Traits::State;

    /// Construct a cell
    Cell(const IndexType id, const State initial_state)
    :
        Entity<Self, Traits>(id, initial_state)
    {}
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_CELL_HH
