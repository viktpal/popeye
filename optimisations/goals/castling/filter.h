#if !defined(OPTIMISATIONS_GOALS_CASTLING_FILTER_H)
#define OPTIMISATIONS_GOALS_CASTLING_FILTER_H

#include "solving/machinery/solve.h"

/* This module provides functionality dealing with the attacking side
 * in STCastlingFilter stipulation slices.
 * Slices of this type optimise solving the goal "castling" by
 * testing whether >=1 castling is legal in the final move.
 */

/* Allocate a STCastlingFilter slice.
 * @return index of allocated slice
 */
slice_index alloc_castling_filter_slice(void);

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
void castling_filter_solve(slice_index si);

#endif
