#include "conditions/einstein/anti.h"
#include "pydata.h"
#include "conditions/einstein/einstein.h"
#include "stipulation/pipe.h"
#include "stipulation/stipulation.h"
#include "stipulation/has_solution_type.h"
#include "stipulation/move_player.h"
#include "solving/move_effect_journal.h"
#include "debugging/trace.h"

#include <assert.h>
#include <stdlib.h>

static void adjust(void)
{
  boolean is_capturer[square_h8-square_a1] = { false };
  move_effect_journal_index_type const top = move_effect_journal_top[nbply];
  move_effect_journal_index_type curr;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  assert(move_effect_journal_top[parent_ply[nbply]]<=top);

  einstein_collect_capturers(is_capturer);

  for (curr = move_effect_journal_top[parent_ply[nbply]]; curr!=top; ++curr)
    if (move_effect_journal[curr].type==move_effect_piece_movement
        && (move_effect_journal[curr].reason==move_effect_reason_moving_piece_movement
            || move_effect_journal[curr].reason==move_effect_reason_castling_king_movement
            || move_effect_journal[curr].reason==move_effect_reason_castling_partner_movement)
        && !is_capturer[move_effect_journal[curr].u.piece_movement.from-square_a1])
    {
      square const to = move_effect_journal[curr].u.piece_movement.to;
      piece const substitute = einstein_decrease_piece(e[to]);
      if (e[to]!=substitute)
        move_effect_journal_do_piece_change(move_effect_reason_einstein_chess,
                                            to,substitute);
    }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type anti_einstein_moving_adjuster_solve(slice_index si,
                                                      stip_length_type n)
{
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  adjust();
  result = solve(slices[si].next1,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Instrument slices with move tracers
 */
void stip_insert_anti_einstein_moving_adjusters(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  stip_instrument_moves(si,STAntiEinsteinArrivingAdjuster);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}