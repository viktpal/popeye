#if !defined(PIECES_WALKS_CHINESE_LEAPERS_H)
#define PIECES_WALKS_CHINESE_LEAPERS_H

/* This module implements chinese leaper pieces */

#include "position/board.h"
#include "position/position.h"
#include "pieces/walks/vectors.h"

/* Generate moves for a chinese leaper piece
 * @param kbeg start of range of vector indices to be used
 * @param kend end of range of vector indices to be used
 */
void chinese_leaper_generate_moves(vec_index_type kbeg, vec_index_type kend);

#endif
