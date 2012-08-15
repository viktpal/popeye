#include "conditions/circe/april.h"
#include "conditions/circe/capture_fork.h"
#include "conditions/circe/super.h"
#include "pydata.h"
#include "stipulation/has_solution_type.h"
#include "stipulation/stipulation.h"
#include "debugging/trace.h"

#include <assert.h>
#include <stdlib.h>

boolean is_april_kind[PieceCount+1];

/* Try to solve in n half-moves after a defense.
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @return length of solution found and written, i.e.:
 *            slack_length-2 defense has turned out to be illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type april_chess_fork_attack(slice_index si, stip_length_type n)
{
  PieNam const pi_captured = abs(pprise[nbply]);
  slice_index const next = is_april_kind[pi_captured] ? slices[si].next1 : slices[si].next2;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  result = attack(next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Try to defend after an attacking move
 * When invoked with some n, the function assumes that the key doesn't
 * solve in less than n half moves.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @return <slack_length - no legal defense found
 *         <=n solved  - <=acceptable number of refutations found
 *                       return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - >acceptable number of refutations found
 */
stip_length_type april_chess_fork_defend(slice_index si, stip_length_type n)
{
  square const pi_captured = pprise[nbply];
  slice_index const next = is_april_kind[abs(pi_captured)] ? slices[si].next1 : slices[si].next2;
  stip_length_type result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  result = defend(next,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Instrument a stipulation
 * @param si identifies root slice of stipulation
 */
void stip_insert_april_chess(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_insert_supercirce_rebirth_handlers(si);
  stip_insert_rebirth_avoider(si,STAprilAprilFork);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
