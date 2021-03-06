#if !defined(PIECES_WALKS_WALKS_H)
#define PIECES_WALKS_WALKS_H

/* this module deals with how pieces wakl */

#include "pieces/pieces.h"

/* standard walks
 * Maps King..Pawn to the associated standard walks
 * The standard walk is typically King..Pawn as well, but modified by conditions
 * such as Chinese Chess (aka Leofamily), Cavalier Majeur, Marine Chess and the
 * like.
 */
typedef piece_walk_type standard_walks_type[Bishop+1];
extern standard_walks_type standard_walks;

/* Determine the orthodox counterpart of a walk
 * @param walk walk to be orthodoxised
 * @return unstandardised (i.e. orthodox) counterpart (one of King..Bishop)
 *         or the walk itself if it is not a standard walk
 */
piece_walk_type orthodoxise_walk(piece_walk_type walk);

/* Initialise array standard_walks according to the current fairy conditions
 */
void initalise_standard_walks(void);

#endif
