#if !defined(CONDITIONS_DUELLISTS_H)
#define CONDITIONS_DUELLISTS_H

/* This module implements the condition Duellists */

#include "solving/machinery/solve.h"
#include "solving/move_effect_journal.h"

extern square duellists[nr_sides];

/* Determine the length of a move for the Duellists condition; the higher the
 * value the more likely the move is going to be played.
 * @return a value expressing the precedence of this move
 */
int duellists_measure_length(void);

/* Undo remembering a duellist
 * @param curr identifies the adjustment effect
 */
void move_effect_journal_undo_remember_duellist(move_effect_journal_index_type curr);

/* Redo remembering a duellist
 * @param curr identifies the adjustment effect
 */
void move_effect_journal_redo_remember_duellist(move_effect_journal_index_type curr);

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
void duellists_remember_duellist_solve(slice_index si);

/* Instrument a stipulation for Duellists
 * @param si identifies root slice of stipulation
 */
void stip_insert_duellists(slice_index si);

#endif
