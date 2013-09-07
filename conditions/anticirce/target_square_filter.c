#include "conditions/anticirce/target_square_filter.h"
#include "conditions/anticirce/anticirce.h"
#include "conditions/circe/circe.h"
#include "stipulation/stipulation.h"
#include "stipulation/pipe.h"
#include "solving/move_effect_journal.h"
#include "debugging/trace.h"

#include <assert.h>

/* This module provides slice type STAnticirceTargetSquareFilter
 */

/* Allocate a STAnticirceTargetSquareFilter slice.
 * @param target target square to be reached
 * @return index of allocated slice
 */
slice_index alloc_anticirce_target_square_filter_slice(square target)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  result = alloc_pipe(STAnticirceTargetSquareFilter);
  slices[result].u.goal_handler.goal.type = goal_target;
  slices[result].u.goal_handler.goal.target = target;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Is the piece that has just moved removed from the target square because it is
 * a Kamikaze piece?
 * @param si identifies filter slice
 * @return true iff the piece is removed
 */
static boolean is_mover_removed_from_target(slice_index si)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    move_effect_journal_index_type const top = move_effect_journal_base[nbply];
    move_effect_journal_index_type const movement = top+move_effect_journal_index_offset_movement;
    square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;
    PieceIdType const moving_id = GetPieceId(move_effect_journal[movement].u.piece_movement.movingspec);
    square const sq_eventual_arrival = move_effect_journal_follow_piece_through_other_effects(nbply,moving_id,sq_arrival);
    result = sq_eventual_arrival!=sq_arrival;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            previous_move_is_illegal the move just played (or being played)
 *                                     is illegal
 *            immobility_on_next_move  the moves just played led to an
 *                                     unintended immobility on the next move
 *            <=n+1 length of shortest solution found (n+1 only if in next
 *                                     branch)
 *            n+2 no solution found in this branch
 *            n+3 no solution found in next branch
 */
stip_length_type anticirce_target_square_filter_solve(slice_index si, stip_length_type n)
{
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  if (is_mover_removed_from_target(si))
    result = n+2;
  else
    result = solve(slices[si].next1,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}
