#if !defined(CONDITIONS_MAGIC_SQUARE_H)
#define CONDITIONS_MAGIC_SQUARE_H

#include "conditions/conditions.h"
#include "solving/machinery/solve.h"

/* This module implements the condition Magic squares */

extern ConditionNumberedVariantType magic_square_type;

/* Try to solve in solve_nr_remaining half-moves.
 * @param si slice index
 * @note assigns solve_result the length of solution found and written, i.e.:
 *            previous_move_is_illegal the move just played is illegal
 *            this_move_is_illegal     the move being played is illegal
 *            immobility_on_next_move  the moves just played led to an
 *                                     unintended immobility on the next move
 *            <=n+1 length of shortest solution found (n+1 only if in next
 *                                     branch)
 *            n+2 no solution found in this branch
 *            n+3 no solution found in next branch
 *            (with n denominating solve_nr_remaining)
 */
void magic_square_side_changer_solve(slice_index si);

/* Instrument slices with magic square side changers
 */
void solving_insert_magic_square(slice_index si);

#endif
