#include "conditions/brunner.h"
#include "solving/has_solution_type.h"
#include "solving/temporary_hacks.h"
#include "solving/check.h"
#include "solving/observation.h"
#include "solving/machinery/solve.h"
#include "solving/move_generator.h"
#include "solving/fork.h"
#include "stipulation/structure_traversal.h"
#include "stipulation/pipe.h"
#include "debugging/trace.h"
#include "debugging/assert.h"

/* Validate an observation according to Brunner Chess
 * @return true iff the observation is valid
 */
boolean brunner_validate_observation(slice_index si)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = (fork_solve(temporary_hack_brunner_check_defense_finder[trait[nbply]],
                       length_unspecified)
            ==next_move_has_solution);

  PUSH_OBSERVATION_TARGET_AGAIN(nbply);

  if (result)
    result = pipe_validate_observation_recursive_delegate(si);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static void remove_first_generator(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  pipe_remove(si);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remove_move_generator(Side side)
{
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side,"");
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,0);
  stip_structure_traversal_override_single(&st,STMoveGenerator,&remove_first_generator);
  stip_traverse_structure(SLICE_NEXT2(temporary_hack_brunner_check_defense_finder[side]),
                          &st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Inialise solving in Brunner Chess
 * @param si identifies the root slice of the solving machinery
 */
void brunner_initialise_solving(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  /* we play the king capturing move generated by the observation machinery */
  remove_move_generator(White);
  remove_move_generator(Black);

  stip_instrument_check_validation(si,nr_sides,STBrunnerValidateCheck);
  check_no_king_is_possible();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
