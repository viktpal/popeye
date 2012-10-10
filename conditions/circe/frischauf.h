#if !defined(CONDITIONS_FRISCHAUFCIRCE_H)
#define CONDITIONS_FRISCHAUFCIRCE_H

#include "solving/solve.h"

/* This module implements the condition Frischauf Circe */

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type frischauf_promotee_marker_solve(slice_index si,
                                                  stip_length_type n);

/* Instrument slices with promotee markers
 */
void stip_insert_frischauf_promotee_markers(slice_index si);

#endif