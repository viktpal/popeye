#if !defined(PYLEAFD_H)
#define PYLEAFD_H

#include "boolean.h"
#include "pyslice.h"
#include "pydirect.h"
#include "pytable.h"
#include "py.h"

/* This module provides functionality dealing with direct leaf
 * stipulation slices.
 */

/* Find and write post key play
 * @param leaf slice index
 * @return true iff >=1 solution was found
 */
boolean leaf_d_solve_postkey(slice_index leaf);

/* Determine and write threats of a slice
 * @param threats table where to store threats
 * @param leaf index of branch slice
 */
void leaf_d_solve_threats(table threats, slice_index leaf);

/* Determine whether the attacker has reached slice si's goal with his
 * move just played.
 * @param si slice identifier
 * @return true iff the starter reached the goal
 */
boolean leaf_d_has_starter_reached_goal(slice_index si);

/* Determine whether the defender wins after a move by the attacker
 * @param leaf identifies leaf
 * @return true iff the defender wins
 */
boolean leaf_d_does_defender_win(slice_index leaf);

/* Determine whether the defense just played defends against the threats.
 * @param threats table containing the threats
 * @param leaf slice index
 * @return true iff the defense defends against at least one of the
 *         threats
 */
boolean leaf_d_are_threats_refuted(table threats, slice_index leaf);

/* Determine whether there is a solution in a leaf.
 * @param leaf slice index of leaf slice
 * @return whether there is a solution and (to some extent) why not
 */
has_solution_type leaf_d_has_solution(slice_index leaf);

/* Determine and write keys at root level
 * @param leaf leaf's slice index
 * @return true iff >=1 key was found and written
 */
boolean leaf_d_root_solve(slice_index leaf);

/* Solve a slice
 * @param si slice index
 * @param n maximum number of half moves until goal
 * @param n_min minimal number of half moves to try
 * @return number of half moves effectively used
 *         n+2 if no solution was found
 *         (n-slack_length_direct)%2 if the previous move led to a
 *            dead end (e.g. self-check)
 */
stip_length_type leaf_d_solve_in_n(slice_index leaf,
                                   stip_length_type n,
                                   stip_length_type n_min);

/* Write a priori unsolvability (if any) of a leaf (e.g. forced reflex
 * mates)
 * @param leaf leaf's slice index
 */
void leaf_d_write_unsolvability(slice_index leaf);

/* Detect starter field with the starting side if possible. 
 * @param leaf identifies leaf
 * @param same_side_as_root does si start with the same side as root?
 * @return does the leaf decide on the starter?
 */
who_decides_on_starter leaf_d_detect_starter(slice_index leaf,
                                             boolean same_side_as_root);

#endif
