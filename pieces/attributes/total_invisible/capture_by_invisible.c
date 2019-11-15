#include "pieces/attributes/total_invisible/capture_by_invisible.h"
#include "pieces/attributes/total_invisible/consumption.h"
#include "pieces/attributes/total_invisible/decisions.h"
#include "pieces/attributes/total_invisible/taboo.h"
#include "pieces/attributes/total_invisible/revelations.h"
#include "pieces/attributes/total_invisible/uninterceptable_check.h"
#include "pieces/attributes/total_invisible.h"
#include "solving/ply.h"
#include "solving/move_effect_journal.h"
#include "optimisations/orthodox_check_directions.h"
#include "debugging/assert.h"
#include "debugging/trace.h"

static void flesh_out_capture_by_inserted_invisible(piece_walk_type walk_capturing,
                                                    square sq_departure)
{
  Side const side_playing = trait[nbply];
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];
  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_added = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_added = GetPieceId(flags_added);

  TraceFunctionEntry(__func__);
  TraceWalk(walk_capturing);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  // TODO first test allocation, then taboo?

  if (was_taboo(sq_departure,White) || was_taboo(sq_departure,Black) || is_taboo(sq_departure,side_playing))
  {
    REPORT_DECISION_OUTCOME("%s","capturer can't be placed on taboo square");
    REPORT_DEADEND;
    max_decision_level = motivation[id_added].levels.from;
  }
  else
  {
    dynamic_consumption_type const save_consumption = current_consumption;
    if (allocate_flesh_out_unplaced(side_playing))
    {
      Side const side_in_check = trait[nbply-1];
      square const king_pos = being_solved.king_square[side_in_check];

      TraceConsumption();TraceEOL();
      assert(nr_total_invisbles_consumed()<=total_invisible_number);

      ++being_solved.number_of_pieces[side_playing][walk_capturing];
      occupy_square(sq_departure,walk_capturing,flags_added);

      if (is_square_uninterceptably_attacked(side_in_check,king_pos))
      {
        REPORT_DECISION_OUTCOME("%s","capturer would deliver uninterceptable check");
        REPORT_DEADEND;
        max_decision_level = motivation[id_added].levels.from;
        if (max_decision_level<motivation[id_added].levels.walk)
          max_decision_level = motivation[id_added].levels.walk;
      }
      else
      {
        move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
        square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;
        motivation_type const save_motivation = motivation[id_added];

        assert(!TSTFLAG(being_solved.spec[sq_departure],advers(trait[nbply])));

        /* adding the total invisible in the pre-capture effect sounds tempting, but
         * we have to make sure that there was no illegal check from it before this
         * move!
         * NB: this works with illegal checks both from the inserted piece and to
         * the inserted king (afert we restart_from_scratch()).
         */
        assert(move_effect_journal[precapture].type==move_effect_piece_readdition);
        move_effect_journal[precapture].type = move_effect_none;

        /* these were set in regular play already: */
        assert(motivation[id_added].first.acts_when==nbply);
        assert(motivation[id_added].first.purpose==purpose_capturer);
        assert(motivation[id_added].last.acts_when==nbply);
        assert(motivation[id_added].last.purpose==purpose_capturer);
        /* fill in the rest: */
        motivation[id_added].first.on = sq_departure;
        motivation[id_added].last.on = sq_arrival;

        move_effect_journal[movement].u.piece_movement.from = sq_departure;
        /* move_effect_journal[movement].u.piece_movement.to unchanged from regular play */
        move_effect_journal[movement].u.piece_movement.moving = walk_capturing;
        move_effect_journal[movement].u.piece_movement.movingspec = being_solved.spec[sq_departure];

        update_nr_taboos_for_current_move_in_ply(+1);
        restart_from_scratch();
        update_nr_taboos_for_current_move_in_ply(-1);

        motivation[id_added] = save_motivation;

        move_effect_journal[precapture].type = move_effect_piece_readdition;
      }

      empty_square(sq_departure);
      --being_solved.number_of_pieces[side_playing][walk_capturing];

      TraceConsumption();TraceEOL();
    }
    else
    {
      REPORT_DECISION_OUTCOME("%s","capturer can't be allocated");
      REPORT_DEADEND;
      max_decision_level = motivation[id_added].levels.from;
    }

    current_consumption = save_consumption;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_walk_for_capture(piece_walk_type walk_capturing,
                                       square sq_departure)
{
  Side const side_in_check = trait[nbply-1];
  square const king_pos = being_solved.king_square[side_in_check];

  Flags const flags_existing = being_solved.spec[sq_departure];

  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];
  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  PieceIdType const id_random = GetPieceId(move_effect_journal[movement].u.piece_movement.movingspec);

  TraceFunctionEntry(__func__);
  TraceWalk(walk_capturing);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  CLRFLAG(being_solved.spec[sq_departure],advers(trait[nbply]));
  SetPieceId(being_solved.spec[sq_departure],id_random);

  ++being_solved.number_of_pieces[trait[nbply]][walk_capturing];
  replace_walk(sq_departure,walk_capturing);

  if (is_square_uninterceptably_attacked(side_in_check,king_pos))
  {
    PieceIdType const id_existing = GetPieceId(flags_existing);

    REPORT_DECISION_OUTCOME("%s","uninterceptable check from the attempted departure square");
    REPORT_DEADEND;

    max_decision_level = motivation[id_existing].levels.walk;
  }
  else
  {
    move_effect_journal_index_type const precapture = effects_base;

    PieceIdType const id_existing = GetPieceId(flags_existing);

    piece_walk_type const save_moving = move_effect_journal[movement].u.piece_movement.moving;
    Flags const save_moving_spec = move_effect_journal[movement].u.piece_movement.movingspec;
    square const save_from = move_effect_journal[movement].u.piece_movement.from;

    motivation_type const motivation_random = motivation[id_random];

    dynamic_consumption_type const save_consumption = current_consumption;

    replace_moving_piece_ids_in_past_moves(id_existing,id_random,nbply-1);

    motivation[id_random].first = motivation[id_existing].first;
    motivation[id_random].last.on = move_effect_journal[movement].u.piece_movement.to;
    motivation[id_random].last.acts_when = nbply;
    motivation[id_random].last.purpose = purpose_capturer;

    /* deactivate the pre-capture insertion of the moving total invisible since
     * that piece is already on the board
     */
    assert(move_effect_journal[precapture].type==move_effect_piece_readdition);
    move_effect_journal[precapture].type = move_effect_none;

    move_effect_journal[movement].u.piece_movement.moving = walk_capturing;
    move_effect_journal[movement].u.piece_movement.movingspec = being_solved.spec[sq_departure];
    move_effect_journal[movement].u.piece_movement.from = sq_departure;
    /* move_effect_journal[movement].u.piece_movement.to unchanged from regular play */

    update_nr_taboos_for_current_move_in_ply(+1);

    allocate_flesh_out_placed(trait[nbply]);

    restart_from_scratch();

    current_consumption = save_consumption;

    update_nr_taboos_for_current_move_in_ply(-1);

    move_effect_journal[movement].u.piece_movement.moving = save_moving;
    move_effect_journal[movement].u.piece_movement.movingspec = save_moving_spec;
    move_effect_journal[movement].u.piece_movement.from = save_from;

    move_effect_journal[precapture].type = move_effect_piece_readdition;

    motivation[id_random] = motivation_random;

    replace_moving_piece_ids_in_past_moves(id_random,id_existing,nbply-1);
  }

  replace_walk(sq_departure,Dummy);
  --being_solved.number_of_pieces[trait[nbply]][walk_capturing];

  being_solved.spec[sq_departure] = flags_existing;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_with_matching_walk(piece_walk_type walk_capturing,
                                                    square sq_departure)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  PieceIdType const id_random = GetPieceId(move_effect_journal[movement].u.piece_movement.movingspec);
  motivation_type const motivation_random = motivation[id_random];

  Flags const flags_existing = being_solved.spec[sq_departure];
  PieceIdType const id_existing = GetPieceId(flags_existing);

  TraceFunctionEntry(__func__);
  TraceWalk(walk_capturing);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  SetPieceId(being_solved.spec[sq_departure],id_random);
  replace_moving_piece_ids_in_past_moves(id_existing,id_random,nbply-1);

  /* deactivate the pre-capture insertion of the moving total invisible since
   * that piece is already on the board
   */
  assert(move_effect_journal[precapture].type==move_effect_piece_readdition);
  move_effect_journal[precapture].type = move_effect_none;

  move_effect_journal[movement].u.piece_movement.from = sq_departure;
  /* move_effect_journal[movement].u.piece_movement.to unchanged from regular play */
  move_effect_journal[movement].u.piece_movement.moving = walk_capturing;

  update_nr_taboos_for_current_move_in_ply(+1);

  TraceValue("%u",id_random);
  TraceValue("%u",motivation[id_random].first.purpose);
  TraceValue("%u",motivation[id_random].first.acts_when);
  TraceSquare(motivation[id_random].first.on);
  TraceValue("%u",motivation[id_random].last.purpose);
  TraceValue("%u",motivation[id_random].last.acts_when);
  TraceSquare(motivation[id_random].last.on);
  TraceEOL();

  motivation[id_random].first = motivation[id_existing].first;
  motivation[id_random].last.on = move_effect_journal[movement].u.piece_movement.to;
  motivation[id_random].last.acts_when = nbply;
  motivation[id_random].last.purpose = purpose_capturer;

  assert(!TSTFLAG(being_solved.spec[sq_departure],advers(trait[nbply])));
  move_effect_journal[movement].u.piece_movement.movingspec = being_solved.spec[sq_departure];
  recurse_into_child_ply();

  motivation[id_random] = motivation_random;

  update_nr_taboos_for_current_move_in_ply(-1);

  move_effect_journal[precapture].type = move_effect_piece_readdition;

  replace_moving_piece_ids_in_past_moves(id_random,id_existing,nbply-1);
  being_solved.spec[sq_departure] = flags_existing;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_piece_at_end_of_line(piece_walk_type walk_capturing,
                                            square sq_departure)
{
  Flags const flags_existing = being_solved.spec[sq_departure];
  PieceIdType const id_existing = GetPieceId(flags_existing);

  TraceFunctionEntry(__func__);
  TraceWalk(walk_capturing);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  if (motivation[id_existing].last.acts_when<nbply
      || ((motivation[id_existing].last.purpose==purpose_interceptor
           || motivation[id_existing].last.purpose==purpose_capturer)
          && motivation[id_existing].last.acts_when<=nbply))
  {
    move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];
    move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
    PieceIdType const id_random = GetPieceId(move_effect_journal[movement].u.piece_movement.movingspec);

    motivation_type const motivation_existing = motivation[id_existing];
    piece_walk_type const walk_on_board = get_walk_of_piece_on_square(sq_departure);

    TraceValue("%u",id_existing);
    TraceValue("%u",motivation[id_existing].first.purpose);
    TraceValue("%u",motivation[id_existing].first.acts_when);
    TraceSquare(motivation[id_existing].first.on);
    TraceValue("%u",motivation[id_existing].last.purpose);
    TraceValue("%u",motivation[id_existing].last.acts_when);
    TraceSquare(motivation[id_existing].last.on);
    TraceEOL();

    assert(motivation[id_existing].first.purpose!=purpose_none);
    assert(motivation[id_existing].last.purpose!=purpose_none);

    motivation[id_existing].levels = motivation[id_random].levels;
    motivation[id_existing].last.purpose = purpose_none;

    if (walk_on_board==walk_capturing)
    {
      TraceWalk(get_walk_of_piece_on_square(sq_departure));
      TraceValue("%x",being_solved.spec[sq_departure]);
      TraceEOL();

      capture_by_invisible_with_matching_walk(walk_capturing,sq_departure);
    }
    else if (walk_on_board==Dummy)
    {
      TraceWalk(get_walk_of_piece_on_square(sq_departure));
      TraceValue("%x",being_solved.spec[sq_departure]);
      TraceEOL();

      flesh_out_walk_for_capture(walk_capturing,sq_departure);
    }

    motivation[id_existing] = motivation_existing;
  }
  else
  {
    TraceText("the piece was added to later act from its current square\n");
    REPORT_DECISION_OUTCOME("%s","the piece was added to later act from its current square");
    REPORT_DEADEND;
    max_decision_level = motivation[id_existing].levels.from;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_rider_inserted_or_existing(piece_walk_type walk_rider,
                                                            vec_index_type kcurr, vec_index_type kend)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;

  TraceFunctionEntry(__func__);
  TraceWalk(walk_rider);
  TraceFunctionParam("%u",kcurr);
  TraceFunctionParam("%u",kend);
  TraceFunctionParamListEnd();

  TraceSquare(sq_arrival);TraceEOL();

  for (; kcurr<=kend && curr_decision_level<=max_decision_level; ++kcurr)
  {
    square sq_departure;
    for (sq_departure = sq_arrival+vec[kcurr];
         is_square_empty(sq_departure) && curr_decision_level<=max_decision_level;
         sq_departure += vec[kcurr])
    {
      motivation[id_inserted].levels.from = curr_decision_level;
      REPORT_DECISION_SQUARE('>',sq_departure);
      ++curr_decision_level;

      max_decision_level = decision_level_latest;
      motivation[id_inserted].levels.walk = curr_decision_level;
      REPORT_DECISION_WALK('>',walk_rider);
      ++curr_decision_level;
      flesh_out_capture_by_inserted_invisible(walk_rider,sq_departure);
      --curr_decision_level;

      if (curr_decision_level<=max_decision_level)
      {
        max_decision_level = decision_level_latest;
        motivation[id_inserted].levels.walk = curr_decision_level;
        REPORT_DECISION_WALK('>',Queen);
        ++curr_decision_level;
        flesh_out_capture_by_inserted_invisible(Queen,sq_departure);
        --curr_decision_level;
      }

      --curr_decision_level;
    }

    if (is_on_board(sq_departure) && curr_decision_level<=max_decision_level)
    {
      TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],Chameleon));
      TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],trait[nbply]));
      TraceEOL();

      if (TSTFLAG(being_solved.spec[sq_departure],Chameleon)
          && TSTFLAG(being_solved.spec[sq_departure],trait[nbply]))
      {
        Flags const flags_existing = being_solved.spec[sq_departure];
        PieceIdType const id_existing = GetPieceId(flags_existing);
        decision_levels_type const save_levels = motivation[id_existing].levels;

        motivation[id_existing].levels.from = curr_decision_level;
        REPORT_DECISION_SQUARE('>',sq_departure);
        ++curr_decision_level;

        max_decision_level = decision_level_latest;
        motivation[id_existing].levels.walk = curr_decision_level;
        REPORT_DECISION_WALK('>',walk_rider);
        ++curr_decision_level;

        capture_by_piece_at_end_of_line(walk_rider,sq_departure);

        --curr_decision_level;

        if (curr_decision_level<=max_decision_level)
        {
          max_decision_level = decision_level_latest;
          motivation[id_existing].levels.walk = curr_decision_level;
          REPORT_DECISION_WALK('>',Queen);
          ++curr_decision_level;

          capture_by_piece_at_end_of_line(Queen,sq_departure);

          --curr_decision_level;
        }

        --curr_decision_level;

        motivation[id_existing].levels = save_levels;
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_king_at_end_of_line(square sq_departure)
{
  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  assert(TSTFLAG(being_solved.spec[sq_departure],Royal));

  TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],Chameleon));
  TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],trait[nbply]));
  TraceEOL();

  if (TSTFLAG(being_solved.spec[sq_departure],Chameleon)
      && TSTFLAG(being_solved.spec[sq_departure],trait[nbply]))
  {
    move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

    move_effect_journal_index_type const precapture = effects_base;
    Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
    PieceIdType const id_inserted = GetPieceId(flags_inserted);

    Flags const flags_existing = being_solved.spec[sq_departure];
    PieceIdType const id_existing = GetPieceId(flags_existing);

    motivation[id_existing].levels.walk = motivation[id_inserted].levels.walk;

    capture_by_piece_at_end_of_line(King,sq_departure);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_king_inserted_or_existing(void)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;
  move_effect_journal_index_type const king_square_movement = movement+1;
  vec_index_type kcurr;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  motivation[id_inserted].levels.walk = curr_decision_level;
  REPORT_DECISION_WALK('>',King);
  ++curr_decision_level;

  assert(move_effect_journal[precapture].type==move_effect_piece_readdition);
  assert(!TSTFLAG(move_effect_journal[movement].u.piece_movement.movingspec,Royal));
  assert(move_effect_journal[king_square_movement].type==move_effect_none);

  for (kcurr = vec_queen_start;
       kcurr<=vec_queen_end && curr_decision_level<=max_decision_level;
       ++kcurr)
  {
    square const sq_departure = sq_arrival+vec[kcurr];
    Flags const flags_existing = being_solved.spec[sq_departure];
    PieceIdType const id_existing = GetPieceId(flags_existing);
    decision_levels_type const save_levels = motivation[id_existing].levels;

    max_decision_level = decision_level_latest;

    motivation[id_existing].levels.from = curr_decision_level;
    REPORT_DECISION_SQUARE('>',sq_departure);
    ++curr_decision_level;

    move_effect_journal[king_square_movement].type = move_effect_king_square_movement;
    move_effect_journal[king_square_movement].u.king_square_movement.from = sq_departure;
    move_effect_journal[king_square_movement].u.king_square_movement.to = sq_arrival;
    move_effect_journal[king_square_movement].u.king_square_movement.side = trait[nbply];

    if (get_walk_of_piece_on_square(sq_departure)==King
        && sq_departure==being_solved.king_square[trait[nbply]])
    {
      assert(TSTFLAG(being_solved.spec[sq_departure],Royal));
      capture_by_king_at_end_of_line(sq_departure);
    }
    else if (being_solved.king_square[trait[nbply]]==initsquare)
    {
      being_solved.king_square[trait[nbply]] = sq_departure;

      if (is_square_empty(sq_departure))
      {
        assert(!TSTFLAG(move_effect_journal[precapture].u.piece_addition.added.flags,Royal));
        SETFLAG(move_effect_journal[precapture].u.piece_addition.added.flags,Royal);
        flesh_out_capture_by_inserted_invisible(King,sq_departure);
        CLRFLAG(move_effect_journal[precapture].u.piece_addition.added.flags,Royal);
      }
      else if (get_walk_of_piece_on_square(sq_departure)==Dummy)
      {
        assert(!TSTFLAG(being_solved.spec[sq_departure],Royal));
        SETFLAG(being_solved.spec[sq_departure],Royal);
        capture_by_king_at_end_of_line(sq_departure);
        CLRFLAG(being_solved.spec[sq_departure],Royal);
      }

      being_solved.king_square[trait[nbply]] = initsquare;
    }

    move_effect_journal[king_square_movement].type = move_effect_none;

    --curr_decision_level;
    motivation[id_existing].levels = save_levels;
  }

  --curr_decision_level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_leaper_inserted_or_existing(piece_walk_type walk_leaper,
                                                             vec_index_type kcurr, vec_index_type kend)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;

  TraceFunctionEntry(__func__);
  TraceWalk(walk_leaper);
  TraceFunctionParam("%u",kcurr);
  TraceFunctionParam("%u",kend);
  TraceFunctionParamListEnd();

  motivation[id_inserted].levels.walk = curr_decision_level;
  REPORT_DECISION_WALK('>',walk_leaper);
  ++curr_decision_level;

  for (; kcurr<=kend && curr_decision_level<=max_decision_level; ++kcurr)
  {
    square const sq_departure = sq_arrival+vec[kcurr];

    max_decision_level = decision_level_latest;

    if (is_square_empty(sq_departure))
      flesh_out_capture_by_inserted_invisible(walk_leaper,sq_departure);
    else
    {
      TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],Chameleon));
      TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],trait[nbply]));
      TraceEOL();

      if (TSTFLAG(being_solved.spec[sq_departure],Chameleon)
          && TSTFLAG(being_solved.spec[sq_departure],trait[nbply]))
      {
        Flags const flags_existing = being_solved.spec[sq_departure];
        PieceIdType const id_existing = GetPieceId(flags_existing);
        decision_levels_type const save_levels = motivation[id_existing].levels;

        motivation[id_existing].levels.walk = motivation[id_inserted].levels.walk;

        motivation[id_existing].levels.from = curr_decision_level;
        REPORT_DECISION_SQUARE('>',sq_departure);
        ++curr_decision_level;

        capture_by_piece_at_end_of_line(walk_leaper,sq_departure);

        --curr_decision_level;
        motivation[id_existing].levels = save_levels;
      }
    }
  }

  --curr_decision_level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_pawn_inserted_or_existing_one_dir(int dir_horiz)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);

  move_effect_journal_index_type const capture = effects_base+move_effect_journal_index_offset_capture;
  square const sq_capture = move_effect_journal[capture].u.piece_removal.on;
  int const dir_vert = trait[nbply]==White ? -dir_up : -dir_down;
  SquareFlags const promsq = trait[nbply]==White ? WhPromSq : BlPromSq;
  SquareFlags const basesq = trait[nbply]==White ? WhBaseSq : BlBaseSq;
  square const sq_departure = sq_capture+dir_vert+dir_horiz;

  TraceFunctionEntry(__func__);
  TraceValue("%d",dir_horiz);
  TraceFunctionParamListEnd();

  // TODO en passant capture

  if (curr_decision_level<=max_decision_level)
  {
    TraceSquare(sq_departure);TraceEOL();
    if (!TSTFLAG(sq_spec[sq_departure],basesq)
        && !TSTFLAG(sq_spec[sq_departure],promsq))
    {
      max_decision_level = decision_level_latest;

      if (is_square_empty(sq_departure))
        flesh_out_capture_by_inserted_invisible(Pawn,sq_departure);
      else
      {
        TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],Chameleon));
        TraceValue("%u",TSTFLAG(being_solved.spec[sq_departure],trait[nbply]));
        TraceEOL();

        if (TSTFLAG(being_solved.spec[sq_departure],Chameleon)
            && TSTFLAG(being_solved.spec[sq_departure],trait[nbply]))
        {
          Flags const flags_existing = being_solved.spec[sq_departure];
          PieceIdType const id_existing = GetPieceId(flags_existing);
          decision_levels_type const save_levels = motivation[id_existing].levels;

          motivation[id_existing].levels.walk = motivation[id_inserted].levels.walk;

          motivation[id_existing].levels.from = curr_decision_level;
          REPORT_DECISION_SQUARE('>',sq_departure);
          ++curr_decision_level;

          capture_by_piece_at_end_of_line(Pawn,sq_departure);

          --curr_decision_level;
          motivation[id_existing].levels = save_levels;
        }
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_pawn_inserted_or_existing(void)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  motivation[id_inserted].levels.walk = curr_decision_level;
  REPORT_DECISION_WALK('>',Pawn);
  ++curr_decision_level;

  TraceValue("%u",curr_decision_level);
  TraceValue("%u",max_decision_level);
  TraceEOL();

  capture_by_invisible_pawn_inserted_or_existing_one_dir(dir_left);
  capture_by_invisible_pawn_inserted_or_existing_one_dir(dir_right);

  --curr_decision_level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_inserted_or_existing(boolean can_capture)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags_inserted);
  decision_levels_type const levels_inserted = motivation[id_inserted].levels;

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const save_from = move_effect_journal[movement].u.piece_movement.from;
  piece_walk_type const save_moving = move_effect_journal[movement].u.piece_movement.moving;
  Flags const save_moving_spec = move_effect_journal[movement].u.piece_movement.movingspec;

  TraceFunctionEntry(__func__);
  TraceValue("%u",can_capture);
  TraceFunctionParamListEnd();

  assert(move_effect_journal[movement].type==move_effect_piece_movement);

  max_decision_level = decision_level_latest;

  if (being_solved.king_square[trait[nbply]]==initsquare)
    capture_by_invisible_king_inserted_or_existing();

  if (can_capture)
  {
    capture_by_invisible_pawn_inserted_or_existing();
    capture_by_invisible_leaper_inserted_or_existing(Knight,vec_knight_start,vec_knight_end);
    capture_by_invisible_rider_inserted_or_existing(Bishop,vec_bishop_start,vec_bishop_end);
    capture_by_invisible_rider_inserted_or_existing(Rook,vec_rook_start,vec_rook_end);
  }

  move_effect_journal[movement].u.piece_movement.from = save_from;
  move_effect_journal[movement].u.piece_movement.moving = save_moving;
  move_effect_journal[movement].u.piece_movement.movingspec = save_moving_spec;

  motivation[id_inserted].levels = levels_inserted;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_king_for_capture(square sq_departure)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;
  move_effect_journal_index_type const king_square_movement = movement+1;

  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  assert(!TSTFLAG(move_effect_journal[movement].u.piece_movement.movingspec,Royal));
  assert(move_effect_journal[king_square_movement].type==move_effect_none);

  move_effect_journal[king_square_movement].type = move_effect_king_square_movement;
  move_effect_journal[king_square_movement].u.king_square_movement.from = sq_departure;
  move_effect_journal[king_square_movement].u.king_square_movement.to = sq_arrival;
  move_effect_journal[king_square_movement].u.king_square_movement.side = trait[nbply];

  being_solved.king_square[trait[nbply]] = sq_departure;

  assert(!TSTFLAG(being_solved.spec[sq_departure],Royal));
  SETFLAG(being_solved.spec[sq_departure],Royal);
  flesh_out_walk_for_capture(King,sq_departure);
  CLRFLAG(being_solved.spec[sq_departure],Royal);

  being_solved.king_square[trait[nbply]] = initsquare;

  move_effect_journal[king_square_movement].type = move_effect_none;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_dummy_for_capture_non_king(square sq_departure,
                                                 square sq_arrival,
                                                 PieceIdType id_existing)
{
  int const move_square_diff = sq_departure-sq_arrival;

  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceSquare(sq_arrival);
  TraceValue("%u",id_existing);
  TraceFunctionParamListEnd();

  if (CheckDir[Bishop][move_square_diff]==move_square_diff
      && (trait[nbply]==White ? sq_departure<sq_arrival : sq_departure>sq_arrival))
  {
    SquareFlags const promsq = trait[nbply]==White ? WhPromSq : BlPromSq;
    SquareFlags const basesq = trait[nbply]==White ? WhBaseSq : BlBaseSq;

    if (!TSTFLAG(sq_spec[sq_departure],basesq) && !TSTFLAG(sq_spec[sq_departure],promsq))
    {
      motivation[id_existing].levels.walk = curr_decision_level;
      REPORT_DECISION_WALK('>',Pawn);
      ++curr_decision_level;
      flesh_out_walk_for_capture(Pawn,sq_departure);
      --curr_decision_level;
    }

    // TODO en passant capture
  }

  if (curr_decision_level<=max_decision_level)
  {
    if (CheckDir[Knight][move_square_diff]==move_square_diff)
    {
      max_decision_level = decision_level_latest;

      motivation[id_existing].levels.walk = curr_decision_level;
      REPORT_DECISION_WALK('>',Knight);
      ++curr_decision_level;
      flesh_out_walk_for_capture(Knight,sq_departure);
      --curr_decision_level;
    }

    if (curr_decision_level<=max_decision_level)
    {
      int const dir = CheckDir[Bishop][move_square_diff];
      if (dir!=0 && sq_departure==find_end_of_line(sq_arrival,dir))
      {
        max_decision_level = decision_level_latest;

        motivation[id_existing].levels.walk = curr_decision_level;
        REPORT_DECISION_WALK('>',Bishop);
        ++curr_decision_level;

        flesh_out_walk_for_capture(Bishop,sq_departure);

        /* Don't reduce curr_decision_level yet; if posteriority asks for a
         * different walk, Queen won't do. */
        // TODO is this correct when we detect revelations? cf. capture_by_invisible_rider_inserted_or_existing()

        if (curr_decision_level<=max_decision_level)
        {
          max_decision_level = decision_level_latest;

          REPORT_DECISION_WALK('>',Queen);
          ++curr_decision_level;
          flesh_out_walk_for_capture(Queen,sq_departure);
          --curr_decision_level;
        }

        --curr_decision_level;
      }

      if (curr_decision_level<=max_decision_level)
      {
        int const dir = CheckDir[Rook][move_square_diff];
        if (dir!=0 && sq_departure==find_end_of_line(sq_arrival,dir))
        {
          max_decision_level = decision_level_latest;

          motivation[id_existing].levels.walk = curr_decision_level;
          REPORT_DECISION_WALK('>',Rook);
          ++curr_decision_level;

          flesh_out_walk_for_capture(Rook,sq_departure);

          /* Don't reduce curr_decision_level yet; if posteriority asks for a
           * different walk, Queen won't do. */
          // TODO is this correct when we detect revelations? cf. capture_by_invisible_rider_inserted_or_existing()

          if (curr_decision_level<=max_decision_level)
          {
            max_decision_level = decision_level_latest;

            REPORT_DECISION_WALK('>',Queen);
            ++curr_decision_level;
            flesh_out_walk_for_capture(Queen,sq_departure);
            --curr_decision_level;
          }

          --curr_decision_level;
        }
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_dummy_for_capture_king_or_non_king(square sq_departure,
                                                         square sq_arrival,
                                                         PieceIdType id_existing)
{
  int const move_square_diff = sq_departure-sq_arrival;

  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceSquare(sq_arrival);
  TraceValue("%u",id_existing);
  TraceFunctionParamListEnd();

  assert(being_solved.king_square[trait[nbply]]==initsquare);

  if (CheckDir[Queen][move_square_diff]==move_square_diff)
  {
    motivation[id_existing].levels.walk = curr_decision_level;
    REPORT_DECISION_WALK('>',King);
    ++curr_decision_level;
    flesh_out_king_for_capture(sq_departure);
    --curr_decision_level;
  }

  assert(current_consumption.placed[trait[nbply]]>0);

  if (curr_decision_level<=max_decision_level
      && !(nr_total_invisbles_consumed()==total_invisible_number
           && current_consumption.placed[trait[nbply]]==1))
  {
    max_decision_level = decision_level_latest;
    flesh_out_dummy_for_capture_non_king(sq_departure,sq_arrival,id_existing);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_with_defined_walk(piece_walk_type walk_capturer,
                                                   square sq_departure)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  piece_walk_type const save_moving = move_effect_journal[movement].u.piece_movement.moving;
  Flags const save_moving_spec = move_effect_journal[movement].u.piece_movement.movingspec;
  square const save_from = move_effect_journal[movement].u.piece_movement.from;

  Flags const flags_existing = being_solved.spec[sq_departure];
  PieceIdType const id_existing = GetPieceId(flags_existing);

  TraceFunctionEntry(__func__);
  TraceWalk(walk_capturer);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  motivation[id_existing].levels.walk = curr_decision_level;
  REPORT_DECISION_WALK('>',walk_capturer);
  ++curr_decision_level;

  capture_by_invisible_with_matching_walk(walk_capturer,sq_departure);

  move_effect_journal[movement].u.piece_movement.moving = save_moving;
  move_effect_journal[movement].u.piece_movement.movingspec = save_moving_spec;
  move_effect_journal[movement].u.piece_movement.from = save_from;

  --curr_decision_level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void capture_by_invisible_king(square sq_departure)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;

  move_effect_journal_index_type const king_square_movement = movement+1;

  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceFunctionParamListEnd();

  assert(!TSTFLAG(move_effect_journal[movement].u.piece_movement.movingspec,Royal));

  assert(move_effect_journal[king_square_movement].type==move_effect_none);
  move_effect_journal[king_square_movement].type = move_effect_king_square_movement;
  move_effect_journal[king_square_movement].u.king_square_movement.from = sq_departure;
  move_effect_journal[king_square_movement].u.king_square_movement.to = sq_arrival;
  move_effect_journal[king_square_movement].u.king_square_movement.side = trait[nbply];

  assert(sq_departure==being_solved.king_square[trait[nbply]]);
  assert(TSTFLAG(being_solved.spec[sq_departure],Royal));

  capture_by_invisible_with_defined_walk(King,sq_departure);

  move_effect_journal[king_square_movement].type = move_effect_none;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_capture_by_invisible_on(square sq_departure,
                                              boolean is_king_dealt_with)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;

  Flags const flags_existing = being_solved.spec[sq_departure];
  PieceIdType const id_existing = GetPieceId(flags_existing);

  TraceFunctionEntry(__func__);
  TraceSquare(sq_departure);
  TraceValue("%u",is_king_dealt_with);
  TraceFunctionParamListEnd();

  TraceValue("%u",nbply);
  TraceValue("%x",flags_existing);
  TraceEOL();

  TraceValue("%u",id_existing);
  TraceValue("%u",motivation[id_existing].first.purpose);
  TraceValue("%u",motivation[id_existing].first.acts_when);
  TraceSquare(motivation[id_existing].first.on);
  TraceValue("%u",motivation[id_existing].last.purpose);
  TraceValue("%u",motivation[id_existing].last.acts_when);
  TraceSquare(motivation[id_existing].last.on);
  TraceValue("%u",motivation[id_existing].levels.from);
  TraceValue("%u",motivation[id_existing].levels.to);
  TraceValue("%u",motivation[id_existing].levels.side);
  TraceValue("%u",motivation[id_existing].levels.walk);
  TraceWalk(get_walk_of_piece_on_square(motivation[id_existing].last.on));
  TraceValue("%u",GetPieceId(being_solved.spec[motivation[id_existing].last.on]));
  TraceEOL();

  if (motivation[id_existing].last.purpose!=purpose_none
      && TSTFLAG(flags_existing,trait[nbply]))
  {
    piece_walk_type const walk_existing = get_walk_of_piece_on_square(sq_departure);
    motivation_type const motivation_existing = motivation[id_existing];

    assert(motivation[id_existing].first.purpose!=purpose_none);
    assert(motivation[id_existing].last.purpose!=purpose_none);

    motivation[id_existing].levels.from = curr_decision_level;
    REPORT_DECISION_SQUARE('>',sq_departure);
    ++curr_decision_level;

    if (motivation[id_existing].last.acts_when<nbply
        || ((motivation[id_existing].last.purpose==purpose_interceptor
             || motivation[id_existing].last.purpose==purpose_capturer)
            && motivation[id_existing].last.acts_when<=nbply))
    {
      int const move_square_diff = sq_arrival-sq_departure;

      PieceIdType const id_random = GetPieceId(move_effect_journal[movement].u.piece_movement.movingspec);

      motivation[id_existing].last.purpose = purpose_none;
      motivation[id_existing].levels = motivation[id_random].levels;

      max_decision_level = decision_level_latest;

      switch (walk_existing)
      {
        case King:
          if (is_king_dealt_with)
          {
            REPORT_DECISION_OUTCOME("%s","the king has already been dealt with");
            REPORT_DEADEND;
          }
          else if (CheckDir[Queen][move_square_diff]==move_square_diff)
            capture_by_invisible_king(sq_departure);
          else
          {
            REPORT_DECISION_OUTCOME("%s","the piece on the departure square can't reach the arrival square");
            REPORT_DEADEND;
          }
          break;

        case Queen:
        case Rook:
        case Bishop:
        {
          int const dir = CheckDir[walk_existing][move_square_diff];
          if (dir!=0 && sq_departure==find_end_of_line(sq_arrival,-dir))
            capture_by_invisible_with_defined_walk(walk_existing,sq_departure);
          else
          {
            REPORT_DECISION_OUTCOME("%s","the piece on the departure square can't reach the arrival square");
            REPORT_DEADEND;
          }
          break;
        }

        case Knight:
          if (CheckDir[Knight][move_square_diff]==move_square_diff)
            capture_by_invisible_with_defined_walk(Knight,sq_departure);
          else
          {
            REPORT_DECISION_OUTCOME("%s","the piece on the departure square can't reach the arrival square");
            REPORT_DEADEND;
          }
          break;

        case Pawn:
          if ((trait[nbply]==White ? move_square_diff>0 : move_square_diff<0)
              && CheckDir[Bishop][move_square_diff]==move_square_diff)
          {
            SquareFlags const promsq = trait[nbply]==White ? WhPromSq : BlPromSq;
            SquareFlags const basesq = trait[nbply]==White ? WhBaseSq : BlBaseSq;

            if (!TSTFLAG(sq_spec[sq_departure],basesq)
                && !TSTFLAG(sq_spec[sq_departure],promsq))
              capture_by_invisible_with_defined_walk(Pawn,sq_departure);
            // TODO en passant capture
          }
          else
          {
            REPORT_DECISION_OUTCOME("%s","the piece on the departure square can't reach the arrival square");
            REPORT_DEADEND;
          }
          break;

        case Dummy:
          if (CheckDir[Queen][move_square_diff]!=0
              || CheckDir[Knight][move_square_diff]==move_square_diff)
          {
            if (is_king_dealt_with
                || being_solved.king_square[trait[nbply]]!=initsquare)
              flesh_out_dummy_for_capture_non_king(sq_departure,sq_arrival,id_existing);
            else
              flesh_out_dummy_for_capture_king_or_non_king(sq_departure,sq_arrival,id_existing);
          }
          else
          {
            REPORT_DECISION_OUTCOME("%s","the piece on the departure square can't reach the arrival square");
            REPORT_DEADEND;
            // TODO do motivation[id_existing].levels = motivation[id_random].levels later
            // so that we can use motivation[id_existing] here?
            if (static_consumption.king[advers(trait[nbply])]+static_consumption.pawn_victims[advers(trait[nbply])]+1
                >=total_invisible_number)
            {
              /* move our single piece to a different square
               * or let another piece be our single piece */
              max_decision_level = motivation_existing.levels.to;
              if (max_decision_level<motivation_existing.levels.side)
                max_decision_level = motivation_existing.levels.side;
            }
          }
          break;

        default:
          assert(0);
          break;
      }
    }
    else
    {
      TraceText("the piece was added to later act from its current square\n");
      REPORT_DECISION_OUTCOME("%s","the piece was added to later act from its current square");
      REPORT_DEADEND;
      max_decision_level = motivation[id_existing].levels.from;
    }

    motivation[id_existing] = motivation_existing;

    --curr_decision_level;
  }
  else if (motivation[id_existing].first.acts_when==nbply
           && motivation[id_existing].first.purpose==purpose_interceptor)
  {
    // TODO how can this happen, and how should we deal with it?
    REPORT_DECISION_SQUARE('>',sq_departure);
    ++curr_decision_level;
    REPORT_DECISION_OUTCOME("%s","revelation of interceptor is violated");
    --curr_decision_level;
    REPORT_DEADEND;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void flesh_out_capture_by_invisible_walk_by_walk(void)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  {
    dynamic_consumption_type const save_consumption = current_consumption;

    if (allocate_flesh_out_unplaced(trait[nbply]))
    {
      current_consumption = save_consumption;
      /* no problem - we can simply insert a capturer */
      TraceText("we can insert a capturer if needed\n");
      capture_by_invisible_inserted_or_existing(true);
    }
    else
    {
      boolean can_king_be_inserted;

      TraceText("we can't just insert a capturer\n");

      current_consumption = save_consumption;

      can_king_be_inserted = (being_solved.king_square[trait[nbply]]==initsquare
                              && current_consumption.claimed[trait[nbply]]);

      TraceValue("%u",can_king_be_inserted);TraceEOL();

      if (can_king_be_inserted)
      {
        /* no problem - we can simply insert a capturing king */
        move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

        move_effect_journal_index_type const precapture = effects_base;
        Flags const flags_inserted = move_effect_journal[precapture].u.piece_addition.added.flags;
        PieceIdType const id_inserted = GetPieceId(flags_inserted);
        decision_levels_type const levels_inserted = motivation[id_inserted].levels;

        move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
        square const save_from = move_effect_journal[movement].u.piece_movement.from;
        piece_walk_type const save_moving = move_effect_journal[movement].u.piece_movement.moving;
        Flags const save_moving_spec = move_effect_journal[movement].u.piece_movement.movingspec;

        assert(move_effect_journal[movement].type==move_effect_piece_movement);

        max_decision_level = decision_level_latest;

        motivation[id_inserted].levels.walk = curr_decision_level;
        motivation[id_inserted].levels.from = curr_decision_level+1;

        capture_by_invisible_king_inserted_or_existing();

        move_effect_journal[movement].u.piece_movement.from = save_from;
        move_effect_journal[movement].u.piece_movement.moving = save_moving;
        move_effect_journal[movement].u.piece_movement.movingspec = save_moving_spec;

        motivation[id_inserted].levels = levels_inserted;
      }

      {
        PieceIdType id;
        for (id = get_top_visible_piece_id()+1;
             id<=get_top_invisible_piece_id() && curr_decision_level<=max_decision_level;
             ++id)
          flesh_out_capture_by_invisible_on(motivation[id].last.on,
                                            can_king_be_inserted);
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void flesh_out_capture_by_invisible(void)
{
  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];

  move_effect_journal_index_type const precapture = effects_base;
  move_effect_journal_index_type const capture = effects_base+move_effect_journal_index_offset_capture;
  piece_walk_type const save_removed_walk = move_effect_journal[capture].u.piece_removal.walk;
  Flags const save_removed_spec = move_effect_journal[capture].u.piece_removal.flags;
  square const sq_capture = move_effect_journal[capture].u.piece_removal.on;
  Flags const flags = move_effect_journal[precapture].u.piece_addition.added.flags;
  PieceIdType const id_inserted = GetPieceId(flags);
  decision_levels_type const save_levels = motivation[id_inserted].levels;

  REPORT_DECISION_DECLARE(unsigned int const save_counter = report_decision_counter);

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceSquare(sq_capture);TraceEOL();
  assert(!is_square_empty(sq_capture));

  motivation[id_inserted].levels.side = decision_level_forever;
  motivation[id_inserted].levels.to = decision_level_forever;

  move_effect_journal[capture].u.piece_removal.walk = get_walk_of_piece_on_square(sq_capture);
  move_effect_journal[capture].u.piece_removal.flags = being_solved.spec[sq_capture];

  flesh_out_capture_by_invisible_walk_by_walk();

  move_effect_journal[capture].u.piece_removal.walk = save_removed_walk;
  move_effect_journal[capture].u.piece_removal.flags = save_removed_spec;

#if defined(REPORT_DECISIONS)
  if (report_decision_counter==save_counter)
  {
    REPORT_DECISION_OUTCOME("%s","no invisible piece found that could capture");
    REPORT_DEADEND;
  }
#endif

  motivation[id_inserted].levels = save_levels;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean is_viable_capturer(PieceIdType id)
{
  ply const ply_capture = nbply+1;
  Side const side_capturing = trait[ply_capture];
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceValue("%u",id);TraceEOL();
  TraceAction(&motivation[id].first);TraceEOL();
  TraceAction(&motivation[id].last);TraceEOL();
  TraceValue("%u",GetPieceId(being_solved.spec[motivation[id].last.on]));
  TraceEOL();

  if ((trait[motivation[id].first.acts_when]!=side_capturing && motivation[id].first.purpose==purpose_random_mover)
      || (trait[motivation[id].last.acts_when]!=side_capturing && motivation[id].last.purpose==purpose_random_mover))
  {
    /* piece belongs to wrong side */
    result = false;
  }
  else if (motivation[id].last.acts_when<=nbply && motivation[id].last.purpose==purpose_none)
  {
    /* piece was captured or merged into a capturer from regular play */
    result = false;
  }
  else if ((motivation[id].last.acts_when==nbply || motivation[id].last.acts_when==ply_capture)
           && motivation[id].last.purpose!=purpose_interceptor)
  {
    /* piece is active for another purpose */
    result = false;
  }
  else if (motivation[id].last.acts_when>ply_capture)
  {
    /* piece will be active after the capture */
    result = false;
  }
  else if (motivation[id].first.on==initsquare)
  {
    /* revealed piece - to be replaced by an "actual" piece */
    result = false;
  }
  else
    result = true;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean is_capture_by_invisible(void)
{
  boolean result;
  ply const ply_capture = nbply+1;
  move_effect_journal_index_type const base = move_effect_journal_base[ply_capture];
  move_effect_journal_index_type const movement = base+move_effect_journal_index_offset_movement;
  square const sq_departure = move_effect_journal[movement].u.piece_movement.from;
  square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  if (ply_capture<=top_ply_of_regular_play
      && sq_departure>=capture_by_invisible
      && is_on_board(sq_arrival))
    result = true;
  else
    result = false;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

boolean is_capture_by_invisible_possible(void)
{
  boolean result = true;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  if (is_capture_by_invisible())
  {
    ply const ply_capture = nbply+1;
    dynamic_consumption_type const save_consumption = current_consumption;

    if (allocate_flesh_out_unplaced(trait[ply_capture]))
    {
      /* no problem - we can simply insert a capturer */
    }
    else
    {
      square const save_king_square = being_solved.king_square[trait[ply_capture]];

      /* pretend that the king is placed; necessary if only captures by the invisble king
       * are possisble */
      being_solved.king_square[trait[ply_capture]] = square_a1;

      current_consumption = save_consumption;

      if (allocate_flesh_out_unplaced(trait[ply_capture]))
      {
        /* no problem - we can simply insert a capturing king */
      }
      else
      {
        move_effect_journal_index_type const effects_base = move_effect_journal_base[ply_capture];

        move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
        square const sq_arrival = move_effect_journal[movement].u.piece_movement.to;
        PieceIdType id;

        TraceSquare(sq_arrival);
        TraceValue("%u",nbply);
        TraceValue("%u",ply_capture);
        TraceEOL();

        /* only captures by existing invisibles are viable - can one of them reach the arrival square at all? */
        result = false; /* not until we have proved it */

        for (id = get_top_visible_piece_id()+1;
             !result && id<=get_top_invisible_piece_id();
             ++id)
          if (is_viable_capturer(id))
          {
            square const on = motivation[id].last.on;
            Flags const spec = being_solved.spec[on];

            assert(GetPieceId(spec)==id);

            if (TSTFLAG(spec,trait[ply_capture]))
            {
              piece_walk_type const walk = get_walk_of_piece_on_square(on);
              int const diff = sq_arrival-on;

              switch (walk)
              {
                case King:
                  if (CheckDir[Queen][diff]==diff)
                    result = true;
                  break;

                case Queen:
                  if (CheckDir[Queen][diff]!=0)
                    result = true;
                  break;

                case Rook:
                  if (CheckDir[Rook][diff]!=0)
                    result = true;
                  break;

                case Bishop:
                  if (CheckDir[Bishop][diff]!=0)
                    result = true;
                  break;

                case Knight:
                  if (CheckDir[Knight][diff]==diff)
                    result = true;
                  break;

                case Pawn:
                  if (CheckDir[Bishop][diff]==diff
                      && (trait[nbply]==White ? diff>0 : diff<0))
                    result = true;
                  break;

                case Dummy:
                  if (CheckDir[Queen][diff]!=0 || CheckDir[Knight][diff]==diff)
                    result = true;
                  break;

                default:
                  assert(0);
                  break;
              }
            }
          }
      }

      being_solved.king_square[trait[ply_capture]] = save_king_square;
    }

    current_consumption = save_consumption;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

void fake_capture_by_invisible(void)
{
  PieceIdType const id_capturer = initialise_motivation(purpose_capturer,capture_by_invisible,
                                                        purpose_capturer,capture_by_invisible);
  ply const save_ply = uninterceptable_check_delivered_in_ply;

  move_effect_journal_index_type const effects_base = move_effect_journal_base[nbply];
  move_effect_journal_index_type const precapture = effects_base;
  move_effect_journal_index_type const capture = effects_base+move_effect_journal_index_offset_capture;
  move_effect_journal_index_type const movement = effects_base+move_effect_journal_index_offset_movement;
  move_effect_journal_entry_type const save_movement_entry = move_effect_journal[movement];

  Side const side = trait[nbply];
  Flags spec = BIT(side)|BIT(Chameleon);

  REPORT_DECISION_DECLARE(unsigned int const save_counter = report_decision_counter);

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  assert(!is_square_empty(uninterceptable_check_delivered_from));

  SetPieceId(spec,id_capturer);

  assert(move_effect_journal[precapture].type==move_effect_none);
  move_effect_journal[precapture].type = move_effect_piece_readdition;
  move_effect_journal[precapture].u.piece_addition.added.on = capture_by_invisible;
  move_effect_journal[precapture].u.piece_addition.added.walk = Dummy;
  move_effect_journal[precapture].u.piece_addition.added.flags = spec;
  move_effect_journal[precapture].u.piece_addition.for_side = side;

  assert(move_effect_journal[capture].type==move_effect_no_piece_removal);
  move_effect_journal[capture].type = move_effect_piece_removal;
  move_effect_journal[capture].u.piece_removal.on = uninterceptable_check_delivered_from;
  move_effect_journal[capture].u.piece_removal.walk = get_walk_of_piece_on_square(uninterceptable_check_delivered_from);
  move_effect_journal[capture].u.piece_removal.flags = being_solved.spec[uninterceptable_check_delivered_from];

  assert(move_effect_journal[movement].type==move_effect_piece_movement);
  move_effect_journal[movement].type = move_effect_piece_movement;
  move_effect_journal[movement].u.piece_movement.from = capture_by_invisible;
  move_effect_journal[movement].u.piece_movement.to = uninterceptable_check_delivered_from;
  move_effect_journal[movement].u.piece_movement.moving = Dummy;
  move_effect_journal[movement].u.piece_movement.movingspec = spec;

  ++being_solved.number_of_pieces[trait[nbply]][Dummy];
  occupy_square(capture_by_invisible,Dummy,spec);

  uninterceptable_check_delivered_from = initsquare;
  uninterceptable_check_delivered_in_ply = ply_nil;

  flesh_out_capture_by_invisible();

  uninterceptable_check_delivered_in_ply = save_ply;
  uninterceptable_check_delivered_from = move_effect_journal[capture].u.piece_removal.on;

  empty_square(capture_by_invisible);
  --being_solved.number_of_pieces[trait[nbply]][Dummy];

  move_effect_journal[movement] = save_movement_entry;
  move_effect_journal[capture].type = move_effect_no_piece_removal;
  move_effect_journal[precapture].type = move_effect_none;

  uninitialise_motivation(id_capturer);

#if defined(REPORT_DECISIONS)
  if (report_decision_counter==save_counter)
  {
    REPORT_DECISION_OUTCOME("%s","no invisible piece found that could capture the uninterceptable check deliverer");
    REPORT_DEADEND;
  }
#endif

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
