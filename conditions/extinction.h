#if !defined(CONDITIONS_EXTINCTION_H)
#define CONDITIONS_EXTINCTION_H

/* Implementation of condition Extinction chess
 */

#include "solving/solve.h"

/* Determine whether a side is in check
 * @param si identifies the check tester
 * @param side_in_check which side?
 * @return true iff side_in_check is in check according to slice si
 */
boolean extinction_check_tester_is_in_check(slice_index si, Side side_in_check);

/* Instrument a stipulation
 * @param si identifies root slice of stipulation
 */
void stip_insert_extinction_chess(slice_index si);

#endif
