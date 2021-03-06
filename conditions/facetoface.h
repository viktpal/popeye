#if !defined(CONDITIONS_FACETOFACE_H)
#define CONDITIONS_FACETOFACE_H

#include "stipulation/stipulation.h"
#include "utilities/boolean.h"
#include "position/board.h"
#include "pieces/pieces.h"
#include "solving/observation.h"

/* This module implements the conditions Face-to-face, Back-to-back and
 * Cheek-to-cheek Chess */

/* Make sure that the observer has the expected walk - confronted or originally
 * @return true iff the observation is valid
 */
boolean facetoface_enforce_observer_walk(slice_index si);

/* Generate moves for a single piece
 * @param identifies generator slice
 */
void facetoface_generate_moves_for_piece(slice_index si);

/* Inialise the solving machinery with Face-to-face Chess
 * @param si identifies root slice of solving machinery
 */
void facetoface_initialise_solving(slice_index si);

/* Generate moves for a single piece
 * @param identifies generator slice
 */
void backtoback_generate_moves_for_piece(slice_index si);


/* Make sure that the observer has the expected walk - confronted or originally
 * @return true iff the observation is valid
 */
boolean backtoback_enforce_observer_walk(slice_index si);


/* Inialise the solving machinery with Back-to-back Chess
 * @param si identifies root slice of solving machinery
 */
void backtoback_initialise_solving(slice_index si);

/* Make sure that the observer has the expected walk - confronted or originally
 * @return true iff the observation is valid
 */
boolean cheektocheek_enforce_observer_walk(slice_index si);

/* Generate moves for a single piece
 * @param identifies generator slice
 */
void cheektocheek_generate_moves_for_piece(slice_index si);

/* Inialise the solving machinery with Face-to-face Chess
 * @param si identifies root slice of solving machinery
 */
void cheektocheek_initialise_solving(slice_index si);

#endif
