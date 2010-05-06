#if !defined(PYDIRCTG_H)
#define PYDIRCTG_H

/* Deals with direct branches
 */

#include "stipulation/battle_play/attack_play.h"

/* Allocate a STDirectDefenderFilter slice
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies slice leading towards goal
 * @return index of allocated slice
 */
slice_index alloc_direct_defender_filter_slice(stip_length_type length,
                                               stip_length_type min_length,
                                               slice_index proxy_to_goal);

/* Try to defend after an attempted key move at root level
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @param max_nr_refutations how many refutations should we look for
 * @return <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - <=max_nr_refutations refutations found
 *         n+4 refuted - >max_nr_refutations refutations found
 */
stip_length_type
direct_defender_filter_root_defend(slice_index si,
                                   stip_length_type n,
                                   stip_length_type n_min,
                                   unsigned int max_nr_refutations);

/* Determine whether there are refutations after an attempted key move
 * at non-root level
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @param max_nr_refutations how many refutations should we look for
 * @return <=n solved  - return value is maximum number of moves
 *                       (incl. defense) needed
 *         n+2 refuted - <=max_nr_refutations refutations found
 *         n+4 refuted - >max_nr_refutations refutations found
 */
stip_length_type
direct_defender_filter_can_defend_in_n(slice_index si,
                                       stip_length_type n,
                                       stip_length_type n_min,
                                       unsigned int max_nr_refutations);

/* Try to defend after an attempted key move at non-root level
 * When invoked with some n, the function assumes that the key doesn't
 * solve in less than n half moves.
 * @param si slice index
 * @param n maximum number of half moves until end state has to be reached
 * @param n_min minimum number of half-moves of interesting variations
 *              (slack_length_battle <= n_min <= slices[si].u.branch.length)
 * @return true iff the defender can defend
 */
stip_length_type direct_defender_filter_defend_in_n(slice_index si,
                                                    stip_length_type n,
                                                    stip_length_type n_min);

/* Insert root slices
 * @param si identifies (non-root) slice
 * @param st address of structure representing traversal
 */
void direct_defender_filter_insert_root(slice_index si,
                                        stip_structure_traversal *st);

/* Find the first postkey slice and deallocate unused slices on the
 * way to it
 * @param si slice index
 * @param st address of structure capturing traversal state
 */
void direct_defender_filter_reduce_to_postkey_play(slice_index si,
                                                   stip_structure_traversal *st);

/* Impose the starting side on a stipulation
 * @param si identifies branch
 * @param st address of structure that holds the state of the traversal
 */
void direct_defense_impose_starter(slice_index si, stip_structure_traversal *st);

/* Instrument a branch with STDirectDefenderFilter slices
 * @param si root of branch to be instrumented
 * @param proxy_to_goal identifies slice leading towards goal
 */
void slice_insert_direct_guards(slice_index si, slice_index proxy_to_goal);

#endif
