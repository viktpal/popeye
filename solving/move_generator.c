#include "solving/move_generator.h"
#include "conditions/annan.h"
#include "conditions/beamten.h"
#include "conditions/central.h"
#include "conditions/disparate.h"
#include "conditions/eiffel.h"
#include "conditions/facetoface.h"
#include "conditions/madrasi.h"
#include "conditions/disparate.h"
#include "conditions/marscirce/marscirce.h"
#include "conditions/marscirce/plus.h"
#include "conditions/messigny.h"
#include "conditions/patrol.h"
#include "conditions/marscirce/phantom.h"
#include "conditions/singlebox/type3.h"
#include "conditions/castling_chess.h"
#include "conditions/exchange_castling.h"
#include "conditions/transmuting_kings/transmuting_kings.h"
#include "conditions/transmuting_kings/super.h"
#include "conditions/transmuting_kings/reflective_kings.h"
#include "conditions/transmuting_kings/vaulting_kings.h"
#include "pieces/attributes/paralysing/paralysing.h"
#include "pieces/walks/generate_moves.h"
#include "solving/single_piece_move_generator.h"
#include "solving/castling.h"
#include "solving/king_move_generator.h"
#include "stipulation/branch.h"
#include "stipulation/pipe.h"
#include "stipulation/proxy.h"
#include "stipulation/binary.h"
#include "solving/temporary_hacks.h"
#include "debugging/measure.h"
#include "solving/pipe.h"
#include "debugging/trace.h"
#include "pieces/pieces.h"
#include "output/plaintext/pieces.h"

#include "debugging/assert.h"
#include <string.h>

move_generation_elmt *curr_generation = &move_generation_stack[toppile];

move_generation_elmt move_generation_stack[toppile + 1];

numecoup current_move[maxply+1];
numecoup current_move_id[maxply+1];

piece_walk_type move_generation_current_walk;

static void write_history_recursive(ply ply)
{
  if (parent_ply[ply]>ply_retro_move)
    write_history_recursive(parent_ply[ply]);

  printf(" %u:",ply);
  WriteSquare(move_generation_stack[CURRMOVE_OF_PLY(ply)].departure);
  printf("-");
  WriteSquare(move_generation_stack[CURRMOVE_OF_PLY(ply)].arrival);
}

void move_generator_write_history(void)
{
  printf("\n");
  write_history_recursive(nbply-1);
  printf("\n");
}

static slice_index const slice_rank_order[] =
{
    STGeneratingMovesForPiece,
    STSingleBoxType3TMovesForPieceGenerator,
    STMadrasiMovesForPieceGenerator,
    STEiffelMovesForPieceGenerator,
    STDisparateMovesForPieceGenerator,
    STParalysingMovesForPieceGenerator,
    STUltraPatrolMovesForPieceGenerator,
    STCentralMovesForPieceGenerator,
    STBeamtenMovesForPieceGenerator,
    STCastlingGenerator,
    STAnnanMovesForPieceGenerator,
    STFaceToFaceMovesForPieceGenerator,
    STBackToBackMovesForPieceGenerator,
    STCheekToCheekMovesForPieceGenerator,
    STVaultingKingsMovesForPieceGenerator,
    STTransmutingKingsMovesForPieceGenerator,
    STSuperTransmutingKingsMovesForPieceGenerator,
    STReflectiveKingsMovesForPieceGenerator,
    STCastlingChessMovesForPieceGenerator,
    STPlatzwechselRochadeMovesForPieceGenerator,
    STMessignyMovesForPieceGenerator,
    STPhantomMovesForPieceGenerator,
    STMovesForPieceGeneratorCaptureNoncaptureSeparator,
    STGeneratingNoncapturesForPiece,
    STGeneratingCapturesForPiece,
    STMarsCirceRememberNoRebirth,
    STMarsCirceGenerateFromRebirthSquare,
    STPlusAdditionalCapturesForPieceGenerator,
    STMoveGeneratorRejectCaptures,
    STMoveGeneratorRejectNoncaptures,
    STGeneratingCapturesAndNoncapturesForPiece,
    STMovesForPieceBasedOnWalkGenerator,
    STTrue
};

enum
{
  nr_slice_rank_order_elmts = sizeof slice_rank_order / sizeof slice_rank_order[0]
};

static void move_generation_branch_insert_slices_impl(slice_index generating,
                                                      slice_index const prototypes[],
                                                      unsigned int nr_prototypes,
                                                      slice_index base)
{
  stip_structure_traversal st;
  branch_slice_insertion_state_type state =
  {
    prototypes,nr_prototypes,
    slice_rank_order, nr_slice_rank_order_elmts,
    branch_slice_rank_order_nonrecursive,
    0,
    generating,
    0
  };

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",generating);
  TraceFunctionParamListEnd();

  state.base_rank = get_slice_rank(slices[base].type,&state);
  assert(state.base_rank!=no_slice_rank);
  init_slice_insertion_traversal(&st,&state,stip_traversal_context_intro);
  stip_traverse_structure_children_pipe(generating,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Insert slices into a move generation branch.
 * The inserted slices are copies of the elements of prototypes; the elements of
 * prototypes are deallocated by help_branch_insert_slices().
 * Each slice is inserted at a position that corresponds to its predefined rank.
 * @param si identifies starting point of insertion
 * @param prototypes contains the prototypes whose copies are inserted
 * @param nr_prototypes number of elements of array prototypes
 */
void move_generation_branch_insert_slices(slice_index si,
                                          slice_index const prototypes[],
                                          unsigned int nr_prototypes)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",nr_prototypes);
  TraceFunctionParamListEnd();

  move_generation_branch_insert_slices_impl(si,prototypes,nr_prototypes,si);
  deallocate_slice_insertion_prototypes(prototypes,nr_prototypes);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

typedef struct
{
    Side side;
    slice_type type;
} insertion_configuration;

static void instrument_generating(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(si,st);

  {
    insertion_configuration const * config = st->param;
    if (config->side==nr_sides || config->side==slices[si].starter)
    {
      slice_index const prototype = alloc_pipe(config->type);
      move_generation_branch_insert_slices_impl(si,&prototype,1,si);
      dealloc_slice(prototype);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Instrument move generation with a slice type
 * @param identifies where to start instrumentation
 * @param side which side (pass nr_sides for both sides)
 * @param type type of slice with which to instrument moves
 */
void solving_instrument_move_generation(slice_index si,
                                        Side side,
                                        slice_type type)
{
  stip_structure_traversal st;
  insertion_configuration config = { side, type };

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,&config);
  stip_structure_traversal_override_single(&st,
                                           STGeneratingMovesForPiece,
                                           &instrument_generating);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void insert_separator(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceValue("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    slice_index const proxy_non_capturing = alloc_proxy_slice();
    slice_index const non_capturing = alloc_pipe(STGeneratingNoncapturesForPiece);
    slice_index const reject_captures = alloc_pipe(STMoveGeneratorRejectCaptures);

    slice_index const proxy_capturing = alloc_proxy_slice();
    slice_index const capturing = alloc_pipe(STGeneratingCapturesForPiece);
    slice_index const reject_non_captures = alloc_pipe(STMoveGeneratorRejectNoncaptures);

    slice_index const generator = alloc_binary_slice(STMovesForPieceGeneratorCaptureNoncaptureSeparator,
                                                     proxy_non_capturing,
                                                     proxy_capturing);

    pipe_link(slices[si].prev,generator);

    pipe_link(proxy_non_capturing,non_capturing);
    pipe_link(non_capturing,reject_captures);
    pipe_link(reject_captures,si);

    pipe_link(proxy_capturing,capturing);
    pipe_link(capturing,reject_non_captures);
    pipe_link(reject_non_captures,si);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Instrument the move generation machinery so that captures and non captures
 * are generated (and can be adapted) separately per piece.
 * @param si root slice of solving machinery
 * @param side side for which to instrument; pass no_side for both sides
 * @note inserts proxy slices STGeneratingNoncapturesForPiece and
 *       STGeneratingCapturesForPiece that can be used for adjusting the move
 *       generation
 */
void move_generator_instrument_for_captures_non_captures_separately(slice_index si,
                                                                    Side side)
{
  stip_structure_traversal st;

  solving_instrument_move_generation(si,
                                     side,
                                     STGeneratingCapturesAndNoncapturesForPiece);

  stip_structure_traversal_init(&st,0);
  stip_structure_traversal_override_single(&st,
                                           STGeneratingCapturesAndNoncapturesForPiece,
                                           &insert_separator);
  stip_traverse_structure(si,&st);
}

static boolean always_reject(numecoup n)
{
  return false;
}

static void reject_captures(slice_index si)
{
  numecoup const base = CURRMOVE_OF_PLY(nbply);
  generate_moves_for_piece(slices[si].next1);
  move_generator_filter_captures(base,&always_reject);
}

static void reject_non_captures(slice_index si)
{
  numecoup const base = CURRMOVE_OF_PLY(nbply);
  generate_moves_for_piece(slices[si].next1);
  move_generator_filter_noncaptures(base,&always_reject);
}

/* Generate moves for a piece with a specific walk from a specific departure
 * square.
 * @note the piece on the departure square need not necessarily have walk p
 */
void generate_moves_for_piece_captures_noncaptures_separately(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  generate_moves_for_piece(slices[si].next1);
  generate_moves_for_piece(slices[si].next2);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Generate moves for a single piece
 * @param identifies generator slice
 */
void generate_moves_for_piece(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceEnumerator(slice_type,slices[si].type,"\n");

  switch (slices[si].type)
  {
    case STSingleBoxType3TMovesForPieceGenerator:
      singleboxtype3_generate_moves_for_piece(si);
      break;

    case STMadrasiMovesForPieceGenerator:
      madrasi_generate_moves_for_piece(si);
      break;

    case STEiffelMovesForPieceGenerator:
      eiffel_generate_moves_for_piece(si);
      break;

    case STDisparateMovesForPieceGenerator:
      disparate_generate_moves_for_piece(si);
      break;

    case STParalysingMovesForPieceGenerator:
      paralysing_generate_moves_for_piece(si);
      break;

    case STUltraPatrolMovesForPieceGenerator:
      ultrapatrol_generate_moves_for_piece(si);
      break;

    case STCentralMovesForPieceGenerator:
      central_generate_moves_for_piece(si);
      break;

    case STBeamtenMovesForPieceGenerator:
      beamten_generate_moves_for_piece(si);
      break;

    case STPhantomMovesForPieceGenerator:
      phantom_generate_moves_for_piece(si);
      break;

    case STPlusAdditionalCapturesForPieceGenerator:
      plus_generate_additional_captures_for_piece(si);
      break;

    case STMovesForPieceGeneratorCaptureNoncaptureSeparator:
      generate_moves_for_piece_captures_noncaptures_separately(si);
      break;

    case STMarsCirceRememberNoRebirth:
      marscirce_remember_no_rebirth(si);
      break;

    case STMarsCirceGenerateFromRebirthSquare:
      marscirce_generate_from_rebirth_square(si);
      break;

    case STMoveGeneratorRejectCaptures:
      reject_captures(si);
      break;

    case STMoveGeneratorRejectNoncaptures:
      reject_non_captures(si);
      break;

    case STVaultingKingsMovesForPieceGenerator:
      vaulting_kings_generate_moves_for_piece(si);
      break;

    case STTransmutingKingsMovesForPieceGenerator:
      transmuting_kings_generate_moves_for_piece(si);
      break;

    case STSuperTransmutingKingsMovesForPieceGenerator:
      supertransmuting_kings_generate_moves_for_piece(si);
      break;

    case STReflectiveKingsMovesForPieceGenerator:
      reflective_kings_generate_moves_for_piece(si);
      break;

    case STCastlingChessMovesForPieceGenerator:
      castlingchess_generate_moves_for_piece(si);
      break;

    case STPlatzwechselRochadeMovesForPieceGenerator:
      exchange_castling_generate_moves_for_piece(si);
      break;

    case STCastlingGenerator:
      castling_generator_generate_castling(si);
      break;

    case STMessignyMovesForPieceGenerator:
      messigny_generate_moves_for_piece(si);
      break;

    case STAnnanMovesForPieceGenerator:
      annan_generate_moves_for_piece(si);
      break;

    case STFaceToFaceMovesForPieceGenerator:
      facetoface_generate_moves_for_piece(si);
      break;

    case STBackToBackMovesForPieceGenerator:
      backtoback_generate_moves_for_piece(si);
      break;

    case STCheekToCheekMovesForPieceGenerator:
      cheektocheek_generate_moves_for_piece(si);
      break;

    case STMovesForPieceBasedOnWalkGenerator:
      generate_moves_for_piece_based_on_walk();
      break;

    default:
      assert(0);
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}


/* Allocate a STMoveGenerator slice.
 * @return index of allocated slice
 */
slice_index alloc_move_generator_slice(void)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  result = alloc_pipe(STMoveGenerator);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static void genmove(void)
{
  unsigned int i;
  square square_h = square_h8;
  Side const side = trait[nbply];

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  for (i = nr_rows_on_board; i>0; i--, square_h += dir_down)
  {
    unsigned int j;
    curr_generation->departure = square_h;
    for (j = nr_files_on_board; j>0; j--)
    {
      if (TSTFLAG(spec[curr_generation->departure],side))
      {
        TraceSquare(curr_generation->departure);TraceEOL();
        move_generation_current_walk = get_walk_of_piece_on_square(curr_generation->departure);
        generate_moves_for_piece(slices[temporary_hack_move_generator[side]].next2);
      }
      curr_generation->departure += dir_left;
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

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
void move_generator_solve(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  nextply(slices[si].starter);
  genmove();
  pipe_solve_delegate(si);
  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void insert_move_generator(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(si,st);

  {
    slice_index const prototype = alloc_move_generator_slice();
    branch_insert_slices_contextual(si,st->context,&prototype,1);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void insert_single_piece_move_generator(slice_index si,
                                              stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children_pipe(si,st);

  {
    slice_index const proto = alloc_single_piece_move_generator_slice();
    branch_insert_slices(slices[si].next2,&proto,1);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void remove_move_generator(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_traverse_structure_children(si,st);

  {
    slice_index const generator = branch_find_slice(STMoveGenerator,
                                                    slices[si].next2,
                                                    stip_traversal_context_intro);
    assert(generator!=no_slice);
    pipe_remove(generator);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static structure_traversers_visitor const solver_inserters[] =
{
  { STGeneratingMoves,                        &insert_move_generator                 },
  { STBrunnerDefenderFinder,                  &stip_traverse_structure_children_pipe },
  { STKingCaptureLegalityTester,              &stip_traverse_structure_children_pipe },
  { STMoveLegalityTester,                     &stip_traverse_structure_children_pipe },
  { STCageCirceNonCapturingMoveFinder,        &insert_single_piece_move_generator    },
  { STTakeMakeCirceCollectRebirthSquaresFork, &insert_single_piece_move_generator    },
  { STCastlingIntermediateMoveLegalityTester, &stip_traverse_structure_children_pipe },
  { STOpponentMovesCounterFork,               &remove_move_generator                 }
};

enum
{
  nr_solver_inserters = sizeof solver_inserters / sizeof solver_inserters[0]
};

/* Instrument the solving machinery with move generator slices
 * @param si identifies root the solving machinery
 */
void solving_insert_move_generators(slice_index si)
{
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,0);
  stip_structure_traversal_override(&st,solver_inserters,nr_solver_inserters);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Only keep generated moves that fulfill some criterion
 * @param start identifies last move on stack that the criterion will not be applied to
 * @param criterion to be fulfilled by moves kept
 */
void move_generator_filter_moves(numecoup start,
                                 move_filter_criterion_type criterion)
{
  numecoup i;
  numecoup new_top = start;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",start);
  TraceFunctionParamListEnd();

  for (i = start+1; i<=CURRMOVE_OF_PLY(nbply); ++i)
    if ((*criterion)(i))
    {
      ++new_top;
      move_generation_stack[new_top] = move_generation_stack[i];
    }

  SET_CURRMOVE(nbply,new_top);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Only keep generated captures that fulfill some criterion; non-captures are all kept
 * @param start identifies last move on stack that the criterion will not be applied to
 * @param criterion to be fulfilled by moves kept
 */
void move_generator_filter_captures(numecoup start,
                                    move_filter_criterion_type criterion)
{
  numecoup i;
  numecoup new_top = start;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",start);
  TraceFunctionParamListEnd();

  for (i = start+1; i<=CURRMOVE_OF_PLY(nbply); ++i)
    if (is_square_empty(move_generation_stack[i].capture) || (*criterion)(i))
    {
      ++new_top;
      move_generation_stack[new_top] = move_generation_stack[i];
    }

  SET_CURRMOVE(nbply,new_top);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Only keep generated non-captures that fulfill some criterion; captures are all kept
 * @param start identifies last move on stack that the criterion will not be applied to
 * @param criterion to be fulfilled by moves kept
 */
void move_generator_filter_noncaptures(numecoup start,
                                       move_filter_criterion_type criterion)
{
  numecoup i;
  numecoup new_top = start;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",start);
  TraceFunctionParamListEnd();

  for (i = start+1; i<=CURRMOVE_OF_PLY(nbply); ++i)
    if (!is_square_empty(move_generation_stack[i].capture) || (*criterion)(i))
    {
      ++new_top;
      move_generation_stack[new_top] = move_generation_stack[i];
    }

  SET_CURRMOVE(nbply,new_top);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Invert the order of the moves generated for a ply
 * @param ply the ply
 */
void move_generator_invert_move_order(ply ply)
{
  unsigned int const nr_moves = current_move[ply]-current_move[ply-1];
  numecoup hi = CURRMOVE_OF_PLY(ply);
  numecoup low = hi-nr_moves+1;

  while (low<hi)
  {
    move_generation_elmt const temp = move_generation_stack[low];
    move_generation_stack[low] = move_generation_stack[hi];
    move_generation_stack[hi] = temp;

    ++low;
    --hi;
  }
}

void pop_move(void)
{
  assert(current_move[nbply]>0);
  --current_move[nbply];
}

DEFINE_COUNTER(add_to_move_generation_stack)

void push_move(void)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  INCREMENT_COUNTER(add_to_move_generation_stack);

  assert(current_move[nbply]<toppile);

  TraceSquare(curr_generation->departure);
  TraceSquare(curr_generation->arrival);
  TraceEOL();

  curr_generation->capture = curr_generation->arrival;
  ++current_move[nbply];
  move_generation_stack[CURRMOVE_OF_PLY(nbply)] = *curr_generation;
  move_generation_stack[CURRMOVE_OF_PLY(nbply)].id = current_move_id[nbply];
  ++current_move_id[nbply];
  TraceValue("%u\n",CURRMOVE_OF_PLY(nbply));

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void push_move_capture_extra(square sq_capture)
{
  TraceFunctionEntry(__func__);
  TraceSquare(sq_capture);
  TraceFunctionParamListEnd();

  INCREMENT_COUNTER(add_to_move_generation_stack);

  assert(current_move[nbply]<toppile);

  TraceSquare(curr_generation->departure);
  TraceSquare(curr_generation->arrival);
  TraceEOL();

  curr_generation->capture = sq_capture;
  ++current_move[nbply];
  move_generation_stack[CURRMOVE_OF_PLY(nbply)] = *curr_generation;
  move_generation_stack[CURRMOVE_OF_PLY(nbply)].id = current_move_id[nbply];
  ++current_move_id[nbply];
  TraceValue("%u\n",CURRMOVE_OF_PLY(nbply));

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void push_special_move(square sq_special)
{
  TraceFunctionEntry(__func__);
  TraceValue("%u",sq_special);
  TraceFunctionParamListEnd();

  INCREMENT_COUNTER(add_to_move_generation_stack);

  assert(current_move[nbply]<toppile);

  TraceSquare(curr_generation->departure);
  TraceSquare(curr_generation->arrival);
  TraceEOL();

  curr_generation->capture = sq_special;
  ++current_move[nbply];
  move_generation_stack[CURRMOVE_OF_PLY(nbply)] = *curr_generation;
  move_generation_stack[CURRMOVE_OF_PLY(nbply)].id = current_move_id[nbply];
  ++current_move_id[nbply];
  TraceValue("%u\n",CURRMOVE_OF_PLY(nbply));

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void push_move_copy(numecoup original)
{
  ++current_move[nbply];
  move_generation_stack[CURRMOVE_OF_PLY(nbply)] = move_generation_stack[original];
}

void push_observation_target(square sq_target)
{
  TraceFunctionEntry(__func__);
  TraceSquare(sq_target);
  TraceFunctionParamListEnd();

  ++current_move[nbply];
  move_generation_stack[CURRMOVE_OF_PLY(nbply)].capture = sq_target;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void replace_observation_target(square sq_target)
{
  TraceFunctionEntry(__func__);
  TraceSquare(sq_target);
  TraceFunctionParamListEnd();

  move_generation_stack[CURRMOVE_OF_PLY(nbply)].capture = sq_target;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void pop_all(void)
{
  current_move[nbply] = current_move[nbply-1];
}

typedef unsigned int mark_type;

static mark_type square_marks[square_h8+1] = { 0 };
static mark_type current_mark = 0;

static boolean is_not_duplicate(numecoup n)
{
  square const sq_arrival = move_generation_stack[n].arrival;
  if (square_marks[sq_arrival]==current_mark)
    return false;
  else
  {
    square_marks[sq_arrival] = current_mark;
    return true;
  }
}

/* Remove duplicate moves generated for a single piece.
 * @param last_move_of_prev_piece index of last move of previous piece
 */
void remove_duplicate_moves_of_single_piece(numecoup last_move_of_prev_piece)
{
  if (current_mark==UINT_MAX)
  {
    square i;
    for (i = square_a1; i!=square_h8; ++i)
      square_marks[i] = 0;

    current_mark = 1;
  }
  else
    ++current_mark;

  move_generator_filter_moves(last_move_of_prev_piece,&is_not_duplicate);
}

/* Priorise a move in the move generation stack
 * @param priorised index in the move generation stack of the move to be
 *                  priorised
 */
void move_generator_priorise(numecoup priorised)
{
  /* we move the priorised move one position too far and then shift back one
   * move too many */
  numecoup const one_too_far = CURRMOVE_OF_PLY(nbply)+1;
  move_generation_stack[one_too_far] = move_generation_stack[priorised];
  memmove(&move_generation_stack[priorised],
          &move_generation_stack[priorised+1],
          (one_too_far-priorised)*sizeof move_generation_stack[0]);
}
