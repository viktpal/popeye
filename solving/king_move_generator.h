#if !defined(SOLVING_KING_MOVE_GENERATOR_H)
#define SOLVING_KING_MOVE_GENERATOR_H

#include "solving/machinery/solve.h"

/* This module provides functionality dealing with the attacking side
 * in STKingMoveGenerator stipulation slices.
 */

/* Allocate a STKingMoveGenerator slice.
 * @return index of allocated slice
 */
slice_index alloc_king_move_generator_slice(void);

/* Generate moves for the king (if any) of a side
 */
void generate_king_moves(void);

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
void king_move_generator_solve(slice_index si);

#endif
