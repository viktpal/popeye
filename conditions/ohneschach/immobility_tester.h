#if !defined(STIPULATION_CONDITION_OHNESCHACH_IMMOBILE_TESTER_H)
#define STIPULATION_CONDITION_OHNESCHACH_IMMOBILE_TESTER_H

#include "solving/solve.h"

/* This module provides functionality dealing with slices that detect
 * whether a side is immobile
 */

extern boolean is_ohneschach_suspended;

/* Optimise Ohneschach immobility tester slices
 * @param si where to start (entry slice into stipulation)
 */
void ohneschach_optimise_immobility_testers(slice_index si);

/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type ohneschach_check_guard_solve(slice_index si,
                                               stip_length_type n);


/* Try to solve in n half-moves.
 * @param si slice index
 * @param n maximum number of half moves
 * @return length of solution found and written, i.e.:
 *            slack_length-2 the move just played or being played is illegal
 *            <=n length of shortest solution found
 *            n+2 no solution found
 */
stip_length_type ohneschach_check_guard_defense_solve(slice_index si,
                                                      stip_length_type n);

#endif
