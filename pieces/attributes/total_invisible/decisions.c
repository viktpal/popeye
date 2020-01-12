#include "pieces/attributes/total_invisible/decisions.h"
#include "pieces/attributes/total_invisible/consumption.h"
#include "solving/ply.h"
#include "solving/move_effect_journal.h"
#include "debugging/assert.h"

#include <stdio.h>
#include <string.h>

//#define REPORT_DECISIONS

decision_level_type curr_decision_level = decision_level_initial;
static decision_level_type max_decision_level = decision_level_latest;

decision_levels_type decision_levels[MaxPieceId+1];

typedef enum
{
  decision_object_side,
  decision_object_insertion,
  decision_object_random_move,
  decision_object_walk,
  decision_object_move_vector,
  decision_object_departure,
  decision_object_arrival,
  decision_object_placement,
  decision_object_king_nomination
} decision_object_type;

typedef struct
{
    ply ply;
    decision_object_type object;
    decision_purpose_type purpose;
    PieceIdType id;
    Side side;
} decision_level_property_type;

static decision_level_property_type decision_level_properties[decision_level_dir_capacity];

unsigned long record_decision_counter;

typedef enum
{
  backtrack_none,
  backtrack_until_level,
  backtrack_failure_to_intercept_illegal_checks_by_invisible,
  backtrack_failure_to_intercept_illegal_checks_by_visible,
  backtrack_failture_to_capture_by_invisible,
  backtrack_failture_to_capture_invisible_by_pawn
} backtrack_type;

static backtrack_type current_backtracking = backtrack_none;

#if defined(REPORT_DECISIONS)

static unsigned long prev_record_decision_counter;

static char const purpose_symbol[] = {
    'x' /* decision_purpose_invisible_capturer_existing */
    , 'X' /* decision_purpose_invisible_capturer_inserted */
    , '#' /* decision_purpose_mating_piece_attacker */
    , '+' /* decision_purpose_illegal_check_interceptor */
    , '>' /* decision_purpose_random_mover_forward */
    , '<' /* decision_purpose_random_mover_backward */
};

/* the Posix compliant version of this function strangely works with non-const character arrays */
static char const *basename(char const *path)
{
  return strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
}

static void report_endline(char const *file, unsigned int line)
{
  printf(" (K:%u+%u x:%u+%u !:%u+%u ?:%u+%u F:%u+%u)"
         , static_consumption.king[White]
         , static_consumption.king[Black]
         , static_consumption.pawn_victims[White]
         , static_consumption.pawn_victims[Black]
         , current_consumption.claimed[White]
         , current_consumption.claimed[Black]
         , current_consumption.placed[White]
         , current_consumption.placed[Black]
         , current_consumption.fleshed_out[White]
         , current_consumption.fleshed_out[Black]
         );
  printf(" - %s:#%d",basename(file),line);
  printf(" - D:%lu\n",record_decision_counter++);
  fflush(stdout);
}

#endif

void initialise_decision_context_impl(char const *file, unsigned int line, char const *context)
{
#if defined(REPORT_DECISIONS)
  printf("\n!%s",context);
  write_history_recursive(top_ply_of_regular_play);
  printf(" - %s:#%d",basename(file),line);
  printf(" - D:%lu",record_decision_counter);
  printf(" - %lu",record_decision_counter-prev_record_decision_counter);
  prev_record_decision_counter = record_decision_counter;
  ++record_decision_counter;
  move_numbers_write_history(top_ply_of_regular_play+1);
  fflush(stdout);

  prev_record_decision_counter = record_decision_counter;
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;
}

void record_decision_for_inserted_invisible(PieceIdType id)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",id);
  TraceFunctionParamListEnd();

  decision_levels[id].side = curr_decision_level;
  decision_levels[id].walk = curr_decision_level;
  decision_levels[id].to = curr_decision_level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

void push_decision_random_move_impl(char const *file, unsigned int line, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u TI~-~",purpose_symbol[purpose],nbply);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_random_move;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].side = trait[nbply];

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;
}

decision_level_type push_decision_departure_impl(char const *file, unsigned int line, PieceIdType id, square pos, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  WriteSquare(&output_plaintext_engine,
              stdout,
              pos);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_departure;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;

  if (purpose==decision_purpose_illegal_check_interceptor)
  {
    assert(curr_decision_level>0);
    assert(decision_level_properties[curr_decision_level-1].object==decision_object_side);
    assert(decision_level_properties[curr_decision_level-1].purpose==decision_purpose_illegal_check_interceptor);
    assert(decision_level_properties[curr_decision_level-1].id==id);
    decision_level_properties[curr_decision_level].side = decision_level_properties[curr_decision_level-1].side;
  }
  else
    decision_level_properties[curr_decision_level].side = trait[nbply];

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

// TODO  do we still need to do record decisions regarding move vectors?
decision_level_type push_decision_move_vector_impl(char const *file, unsigned int line, PieceIdType id, int direction, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  printf("direction:%d",direction);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_move_vector;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;

  if (purpose==decision_purpose_illegal_check_interceptor)
  {
    assert(curr_decision_level>0);
    assert(decision_level_properties[curr_decision_level-1].object==decision_object_side);
    assert(decision_level_properties[curr_decision_level-1].purpose==decision_purpose_illegal_check_interceptor);
    assert(decision_level_properties[curr_decision_level-1].id==id);
    decision_level_properties[curr_decision_level].side = decision_level_properties[curr_decision_level-1].side;
  }
  else
    decision_level_properties[curr_decision_level].side = trait[nbply];

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

decision_level_type push_decision_arrival_impl(char const *file, unsigned int line, PieceIdType id, square pos, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  WriteSquare(&output_plaintext_engine,
              stdout,
              pos);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  assert(purpose==decision_purpose_random_mover_forward
         || purpose==decision_purpose_random_mover_backward);

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_arrival;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;
  decision_level_properties[curr_decision_level].side = trait[nbply];

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

decision_level_type push_decision_placement_impl(char const *file, unsigned int line, PieceIdType id, square pos, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  WriteSquare(&output_plaintext_engine,
              stdout,
              pos);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  assert(purpose==decision_purpose_illegal_check_interceptor);

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_placement;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;
  decision_level_properties[curr_decision_level].side = no_side;

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

decision_level_type push_decision_side_impl(char const *file, unsigned int line, PieceIdType id, Side side, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  WriteSpec(&output_plaintext_engine,
            stdout,
            BIT(side),
            initsquare,
            true);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_side;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;
  decision_level_properties[curr_decision_level].side = side;

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

decision_level_type push_decision_insertion_impl(char const *file, unsigned int line, PieceIdType id, Side side, decision_purpose_type purpose)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  printf("I");
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_insertion;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;
  decision_level_properties[curr_decision_level].side = side;

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

decision_level_type push_decision_walk_impl(char const *file, unsigned int line,
                                            PieceIdType id,
                                            piece_walk_type walk,
                                            decision_purpose_type purpose,
                                            Side side)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%c %u ",purpose_symbol[purpose],nbply);
  WriteWalk(&output_plaintext_engine,
            stdout,
            walk);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_walk;
  decision_level_properties[curr_decision_level].purpose = purpose;
  decision_level_properties[curr_decision_level].id = id;
  decision_level_properties[curr_decision_level].side = side;

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;

  return curr_decision_level-1;
}

void push_decision_king_nomination_impl(char const *file, unsigned int line, square pos)
{
#if defined(REPORT_DECISIONS)
  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  WriteSpec(&output_plaintext_engine,
            stdout,
            being_solved.spec[pos],
            being_solved.board[pos],
            true);
  WriteWalk(&output_plaintext_engine,
            stdout,
            being_solved.board[pos]);
  WriteSquare(&output_plaintext_engine,
              stdout,
              pos);
  report_endline(file,line);
#endif

  max_decision_level = decision_level_latest;
  current_backtracking = backtrack_none;

  decision_level_properties[curr_decision_level].ply = nbply;
  decision_level_properties[curr_decision_level].object = decision_object_king_nomination;
  decision_level_properties[curr_decision_level].purpose = decision_purpose_king_nomination;
  decision_level_properties[curr_decision_level].side = TSTFLAG(being_solved.spec[pos],White) ? White : Black;

  ++curr_decision_level;
  assert(curr_decision_level<decision_level_dir_capacity);

  ++record_decision_counter;
}

void record_decision_outcome_impl(char const *file, unsigned int line, char const *format, ...)
{
#if defined(REPORT_DECISIONS)
  va_list args;
  va_start(args,format);

  printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
  printf("%u ",nbply);
  vprintf(format,args);
  printf(" - %s:#%d",basename(file),line);
  printf("\n");
  fflush(stdout);

  va_end(args);
#endif
}

static boolean try_to_avoid_insertion[nr_sides] = { false, false };
static ply ply_failure;
static Side side_failure;

void pop_decision(void)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  decision_level_properties[curr_decision_level].ply = 0;
  decision_level_properties[curr_decision_level].side = no_side;
  assert(curr_decision_level>0);
  --curr_decision_level;

  TraceValue("%u",curr_decision_level);
  TraceValue("%u",max_decision_level);
  TraceValue("%u",decision_level_properties[curr_decision_level].ply);
  TraceValue("%u",decision_level_properties[curr_decision_level].purpose);
  TraceValue("%u",decision_level_properties[curr_decision_level].object);
  TraceEnumerator(Side,decision_level_properties[curr_decision_level].side);
  TraceValue("%u",current_backtracking);
  TraceEOL();

  switch (current_backtracking)
  {
    case backtrack_failure_to_intercept_illegal_checks_by_invisible:
      if (decision_level_properties[curr_decision_level].object==decision_object_insertion)
      {
        assert(decision_level_properties[curr_decision_level].side!=no_side);

        /* remember which side may save an insertion */
        try_to_avoid_insertion[decision_level_properties[curr_decision_level].side] = true;

        if (decision_level_properties[curr_decision_level].purpose==decision_purpose_invisible_capturer_inserted)
          try_to_avoid_insertion[advers(decision_level_properties[curr_decision_level].side)] = true;
      }
      break;

    default:
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

boolean has_decision_failed_capture(void)
{
  boolean result = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  switch (current_backtracking)
  {
    case backtrack_failture_to_capture_by_invisible:
    case backtrack_failture_to_capture_invisible_by_pawn:
      TraceEnumerator(Side,side_failure);TraceEOL();
      if (decision_level_properties[max_decision_level].side==side_failure)
        result = true;
      break;

    default:
      break;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean failure_to_intercept_illegal_checks_continue_level(decision_level_type curr_level)
{
  boolean skip = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",curr_level);
  TraceFunctionParamListEnd();

  TraceValue("%u",curr_level);
  TraceValue("%u",ply_failure);
  TraceEnumerator(Side,side_failure);
  TraceValue("%u",decision_level_properties[curr_level].ply);
  TraceValue("%u",decision_level_properties[curr_level].purpose);
  TraceValue("%u",decision_level_properties[curr_level].object);
  TraceValue("%u",decision_level_properties[curr_level].id);
  TraceEnumerator(Side,decision_level_properties[curr_level].side);
  TraceValue("%u",try_to_avoid_insertion[White]);
  TraceValue("%u",try_to_avoid_insertion[Black]);
  TraceEOL();

  assert(decision_level_properties[curr_level].ply!=0);

  switch (decision_level_properties[curr_level].purpose)
  {
    case decision_purpose_random_mover_backward:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          /* try harder.
           * a future decision may select
           * - a walk that allows us to eventually intercept the check
           */
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_departure)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          skip = true;
          /* e.g.
begin
author Ofer Comay
origin Sake tourney 2018, 3rd HM, cooked (and 1 author's solution doesn't deliver mate)
pieces TotalInvisible 3 white ke5 qh8 bc1 pb7c2h4 black rb4e1 ba1f1 sf2
stipulation h#2
option movenum start  1:1:5:17
end

             Ofer Comay
Sake tourney 2018, 3rd HM, cooked (and 1 authors solution doesnt deliver mate)

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .   .   .   .   Q   8
|                                   |
7   .   P   .   .   .   .   .   .   7
|                                   |
6   .   .   .   .   .   .   .   .   6
|                                   |
5   .   .   .   .   K   .   .   .   5
|                                   |
4   .  -R   .   .   .   .   .   P   4
|                                   |
3   .   .   .   .   .   .   .   .   3
|                                   |
2   .   .   P   .   .  -S   .   .   2
|                                   |
1  -B   .   B   .  -R  -B   .   .   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  6 + 5 + 3 TI

!validate_mate 6:TI~-~ 7:TI~-~ 8:TI~-c2 9:Ke5-f6 - total_invisible.c:#521 - D:3365 - 2414
use option start 1:1:5:17 to replay
!  2 + 6 d4 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#171 - D:3366
!   3 + 6 w (K:0+1 x:0+0 !:0+0 ?:1+0 F:0+0) - intercept_illegal_checks.c:#107 - D:3368
!    4 + 6 e4 (K:0+1 x:0+0 !:0+0 ?:1+0 F:0+0) - intercept_illegal_checks.c:#171 - D:3370
!     5 + 6 w (K:0+1 x:0+0 !:0+0 ?:2+0 F:0+0) - intercept_illegal_checks.c:#107 - D:3372
!      6 > 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:2+0 F:0+0) - random_move_by_invisible.c:#579 - D:3374
!       7 > 7 d4 (K:0+1 x:0+0 !:0+1 ?:2+0 F:0+0) - random_move_by_invisible.c:#552 - D:3376
!        8 > 7 P (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#349 - D:3378
!         9 > 7 d5 (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#25 - D:3380
!          10 < 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#1029 - D:3382
!           11 > 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#579 - D:3384
!            12 + 8 d4 (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - intercept_illegal_checks.c:#171 - D:3386
!            12 + 8 c3 (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - intercept_illegal_checks.c:#171 - D:3388
!             13 8 not enough available invisibles of side White for intercepting all illegal checks - intercept_illegal_checks.c:#135
!             13 + 8 b (K:0+1 x:0+0 !:0+0 ?:1+1 F:1+0) - intercept_illegal_checks.c:#107 - D:3390
!              14 x 8 c3 (K:0+1 x:0+0 !:0+0 ?:1+1 F:1+0) - capture_by_invisible.c:#808 - D:3392
!               15 x 8 K (K:0+1 x:0+0 !:0+0 ?:1+1 F:1+0) - capture_by_invisible.c:#215 - D:3394
!                16 < 6 c3 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - random_move_by_invisible.c:#993 - D:3396
!                 17 < 6 b3 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - random_move_by_invisible.c:#623 - D:3398
!                  18 10 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#642

HERE! no need to try other departure squares

!              14 X 8 I (K:0+1 x:0+0 !:0+0 ?:1+1 F:1+0) - capture_by_invisible.c:#1154 - D:3400
           */
        }
      }
      break;

    case decision_purpose_invisible_capturer_existing:
    case decision_purpose_random_mover_forward:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_walk
          || decision_level_properties[curr_level].object==decision_object_arrival)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          /* try harder.
           * a future decision may select
           * - a walk that allows us to eventually intercept the check
           * - an arrival square from where the check can be intercepted
           */
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_departure)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          /* try harder.
           * a future decision may
           * - select a piece that can intercept the check
           */
           /* e.g.
           Michel Caillaud
Sake tourney 2018, 1st HM, cooked (author's solution relies on retro and is not shown)

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .  -R   .  -R  -B   8
|                                   |
7   .   .   .  -B   .   .   .   .   7
|                                   |
6   .   .   .   P  -K   .   .   R   6
|                                   |
5   .   .   .   .   .   .   .   .   5
|                                   |
4   .   .   .   P   .   .   K   .   4
|                                   |
3   .   .   .   .   .   .   .   .   3
|                                   |
2   B   .   .  -P  -Q   .   .   .   2
|                                   |
1   .   .   .   .   .   .   .   .   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  5 + 7 + 3 TI

>   1.TI~-~ TI~-~   2.TI~-~[d5=bR][f3=wR] TI~-~[g7=wP] #

!test_mate 6:TI~-~ 7:TI~-~ 8:TI~-~ 9:TI~-~ - total_invisible.c:#551 - D:50211 - 38436
use option start 1:1:1:1 to replay
!  2 + 6 f3 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#512 - D:50212
!   3 + 6 b (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#475 - D:62222
!    4 + 6 R (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#253 - D:70672
!     5 + 6 g7 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#512 - D:87214
!      6 + 6 b (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#475 - D:88572
!       7 + 6 P (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#253 - D:88574
!        8 > 6 f3 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+2) - random_move_by_invisible.c:#552 - D:88576
!         9 > 6 f1 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+2) - random_move_by_invisible.c:#25 - D:88862
!          10 7 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#644

HERE

!        8 > 6 g7
           */
        }
        else
        {
          /* try harder.
           * a future decision may
           * - select a departure square where this piece intercepted the check before moving
           */
        }
      }
      else
        skip = true;
      break;

    case decision_purpose_invisible_capturer_inserted:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_insertion)
        skip = true;
      else if (decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].side==side_failure)
        {
          // TODO rather than calculating nbply-2, we should backtrack to the last random move of the side
          if (decision_level_properties[curr_level].ply<ply_failure-2)
          {
            /* try harder.
             * a future decision may select
             * - a walk that allows us to eventually intercept the check
             */
            /* e.g.

begin
author Michel Caillaud
origin Sake tourney 2018, 1st prize, corrected
pieces TotalInvisible 2 white kd1 qb2 black kf4 rh1 be1 pe4f5h3
stipulation h#2
option movenum start 4:4:4:21
end

           Michel Caillaud
Sake tourney 2018, 1st prize, corrected

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .   .   .   .   .   8
|                                   |
7   .   .   .   .   .   .   .   .   7
|                                   |
6   .   .   .   .   .   .   .   .   6
|                                   |
5   .   .   .   .   .  -P   .   .   5
|                                   |
4   .   .   .   .  -P  -K   .   .   4
|                                   |
3   .   .   .   .   .   .   .  -P   3
|                                   |
2   .   Q   .   .   .   .   .   .   2
|                                   |
1   .   .   .   K  -B   .   .  -R   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  2 + 6 + 2 TI

!validate_mate 6:Be1-g3 7:TI~-g3 8:Rh1-g1 9:Qb2-b8 - total_invisible.c:#521 - D:165 - 122
use option start 4:4:4:21 to replay
!  2 X 7 I (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - capture_by_invisible.c:#1154 - D:166
!   3 X 7 P (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - capture_by_invisible.c:#517 - D:168
...
!   3 X 7 S (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - capture_by_invisible.c:#456 - D:184
!    4 X 7 h5 (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - capture_by_invisible.c:#49 - D:186
!     5 7 capturer would deliver uninterceptable check - capture_by_invisible.c:#56
!    4 X 7 f1 (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - capture_by_invisible.c:#49 - D:188
!     5 + 8 e1 (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#171 - D:190
!     5 + 8 f1 (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#171 - D:192
!     5 + 8 g1 (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#171 - D:194
!      6 + 8 w (K:0+0 x:0+0 !:0+0 ?:1+0 F:1+0) - intercept_illegal_checks.c:#107 - D:196
!       7 10 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#644

Here! BTW: ply_skip-3 would be too strong

!   3 X 7 B (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - capture_by_invisible.c:#354 - D:198
...
!   3 X 7 R (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - capture_by_invisible.c:#354 - D:218
...
             */
          }
          else
            skip = true;
        }
        else
        {
          /* try harder.
           * a future decision may select
           * - a walk that doesn't deliver check
           *   - from the departure square (if the capture was after ply_failure)
           *   - from the arrival square (if the capture was before ply_failure)
           */
        }
        break;
      }
      else if (decision_level_properties[curr_level].object==decision_object_departure)
      {
        if (decision_level_properties[curr_level].ply>ply_failure)
        {
          if (decision_level_properties[curr_level].side==side_failure)
          {
            /* try harder.
             * a future decision may select
             * - a square where we aren't in check
             */
             /* e.g.

             Ofer Comay
Sake tourney 2018, 3rd HM, cooked (and 1 authors solution doesnt deliver mate)

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .   .   .   .   Q   8
|                                   |
7   .   P   .   .   .   .   .   .   7
|                                   |
6   .   .   .   .   .   .   .   .   6
|                                   |
5   .   .   .   .   K   .   .   .   5
|                                   |
4   .  -R   .   .   .   .   .   P   4
|                                   |
3   .   .   .   .   .   .   .   .   3
|                                   |
2   .   .   P   .   .  -S   .   .   2
|                                   |
1  -B   .   B   .  -R  -B   .   .   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  6 + 5 + 3 TI

!validate_mate 6:TI~-~ 7:TI~-~ 8:TI~-c2 9:Ke5-d4 - total_invisible.c:#521 - D:2960009 - 16400
use option start 1:1:5:15 to replay

!validate_mate 6:TI~-~ 7:TI~-~ 8:TI~-c2 9:Ke5-d4 - total_invisible.c:#521 - D:3365 - 2414
use option start 1:1:5:15 to replay
!  2 + 6 d4 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#171 - D:3366
!   3 + 6 w (K:0+1 x:0+0 !:0+0 ?:1+0 F:0+0) - intercept_illegal_checks.c:#107 - D:3368
!    4 + 6 e4 (K:0+1 x:0+0 !:0+0 ?:1+0 F:0+0) - intercept_illegal_checks.c:#171 - D:3370
!     5 + 6 w (K:0+1 x:0+0 !:0+0 ?:2+0 F:0+0) - intercept_illegal_checks.c:#107 - D:3372
!      6 > 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:2+0 F:0+0) - random_move_by_invisible.c:#579 - D:3374
!       7 > 7 d4 (K:0+1 x:0+0 !:0+1 ?:2+0 F:0+0) - random_move_by_invisible.c:#552 - D:3376
!        8 > 7 P (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#349 - D:3378
...
!        8 > 7 B (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#376 - D:3670
!         9 > 7 c5 (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#25 - D:3672
...
!         9 > 7 c3 (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#25 - D:3816
!          10 < 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#1029 - D:3818
!           11 > 6 TI~-~ (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - random_move_by_invisible.c:#579 - D:3820
!            12 X 8 I (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - capture_by_invisible.c:#1154 - D:3822
!             13 X 8 K (K:0+1 x:0+0 !:0+1 ?:1+0 F:1+0) - capture_by_invisible.c:#393 - D:3824
!              14 X 8 d2 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - capture_by_invisible.c:#49 - D:3826
!               15 < 6 d2 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - random_move_by_invisible.c:#993 - D:3828
!                16 < 6 d1 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - random_move_by_invisible.c:#623 - D:3830
!                 17 7 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#642

HERE

!              14 X 8 b2 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - capture_by_invisible.c:#49 - D:3700
...
!              14 X 8 d1 (K:0+1 x:0+0 !:0+0 ?:1+0 F:1+1) - capture_by_invisible.c:#49 - D:3736

             */
          }
          else
          {
            /* try harder.
             * a future decision may select
             * - a square from where we don't deliver check
             */
          }
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_move_vector)
      {
        if (decision_level_properties[curr_level].ply>ply_failure
            && decision_level_properties[curr_level].side!=side_failure)
        {
          /* try harder.
           * a future decision may select
           * - a move vector from where we don't deliver check
           */
           /* e.g.
begin
author Michel Caillaud
origin Sake tourney 2018, 2nd HM, cooked
pieces TotalInvisible 2 white kf6 rd3 bd2 sd6 pf2f5g5 black ka4 qh1
stipulation h#2
option movenum start 8:11:15:1
end

           Michel Caillaud
  Sake tourney 2018, 2nd HM, cooked

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .   .   .   .   .   8
|                                   |
7   .   .   .   .   .   .   .   .   7
|                                   |
6   .   .   .   S   .   K   .   .   6
|                                   |
5   .   .   .   .   .   P   P   .   5
|                                   |
4  -K   .   .   .   .   .   .   .   4
|                                   |
3   .   .   .   R   .   .   .   .   3
|                                   |
2   .   .   .   B   .   P   .   .   2
|                                   |
1   .   .   .   .   .   .   .  -Q   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  7 + 2 + 2 TI

!validate_mate 6:Qh1-a8 7:Pf2-g3 8:Qa8-e4 9:TI~-e4 - total_invisible.c:#521 - D:3 - 0
use option start 8:11:15:1 to replay
!  2 7 adding victim of capture by pawn - total_invisible.c:#374
!  2 X 9 I (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#1154 - D:4
!   3 X 9 P (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#517 - D:6
!    4 9 capturer can't be placed on taboo square - capture_by_invisible.c:#35
...
!   3 X 9 R (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#354 - D:24
!    4 X 9 direction:1 (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#360 - D:26
!     5 X 9 f4 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:28
!      6 7 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#644
!     5 X 9 g4 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:30
!      6 7 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#644
!     5 X 9 h4 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:32
!      6 7 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#644

HERE

!    4 X 9 direction:2 (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#360 - D:34
...
!    4 X 9 direction:4 (K:0+0 x:0+1 !:0+0 ?:0+0 F:0+1) - capture_by_invisible.c:#360 - D:52
!     5 X 9 e3 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:54
!      6 10 Replaying moves for validation - king_placement.c:#29
!     5 X 9 e2 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:56
!      6 10 Replaying moves for validation - king_placement.c:#29
!     5 X 9 e1 (K:0+0 x:0+1 !:0+0 ?:0+0 F:1+1) - capture_by_invisible.c:#49 - D:58
!      6 10 Replaying moves for validation - king_placement.c:#29
             */
        }
        else
          skip = true;
      }
      else
        skip = true;
      break;

    case decision_purpose_mating_piece_attacker:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_insertion)
        skip = true;
      break;

    default:
      break;
  }

  if (decision_level_properties[curr_level].purpose==decision_purpose_illegal_check_interceptor)
  {
    if (decision_level_properties[curr_level].object==decision_object_side
        || decision_level_properties[curr_level].object==decision_object_walk)
    {
      assert(decision_level_properties[curr_level].side!=no_side);
      if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can insert a victim\n");
#endif

          skip = false;
        }
      }
    }
  }
  else
  {
    assert(decision_level_properties[curr_level].side!=no_side);
    assert(decision_level_properties[curr_level].object!=decision_object_side);
    if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
    {
      if (decision_level_properties[curr_level].object==decision_object_departure
        || decision_level_properties[curr_level].object==decision_object_arrival
        || decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can intercept the check with an insertion\n");
#endif

          skip = false;
        }
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",skip);
  TraceFunctionResultEnd();
  return skip;
}

/* Optimise backtracking considering that we have
 * reached a position where we aren't able to intercept all illegal checks by inserting
 * invisibles.
 * @param side_in_check the side that is in too many illegal checks
 */
void backtrack_from_failure_to_intercept_illegal_check_by_invisible(Side side_in_check)
{
  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_in_check);
  TraceFunctionParamListEnd();

  assert(current_backtracking==backtrack_none);
  assert(max_decision_level==decision_level_latest);

  current_backtracking = backtrack_failure_to_intercept_illegal_checks_by_invisible;
  max_decision_level = curr_decision_level-1;

  try_to_avoid_insertion[Black] = false;
  try_to_avoid_insertion[White] = false;
  ply_failure = nbply;
  side_failure = side_in_check;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Optimise backtracking considering that we have
 * reached a position where we aren't able to intercept all illegal checks delived by
 * visibles by inserting invisibles.
 * @param side_in_check the side that is in too many illegal checks
 */
void backtrack_from_failure_to_intercept_illegal_check_by_visible(Side side_in_check)
{
  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_in_check);
  TraceFunctionParamListEnd();

  assert(current_backtracking==backtrack_none);
  assert(max_decision_level==decision_level_latest);

  current_backtracking = backtrack_failure_to_intercept_illegal_checks_by_visible;
  max_decision_level = curr_decision_level-1;

  try_to_avoid_insertion[Black] = false;
  try_to_avoid_insertion[White] = false;
  ply_failure = nbply;
  side_failure = side_in_check;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean failure_to_capture_by_invisible_continue_level(decision_level_type curr_level)
{
  boolean skip = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",curr_level);
  TraceFunctionParamListEnd();

  TraceValue("%u",curr_level);
  TraceValue("%u",ply_failure);
  TraceEnumerator(Side,side_failure);
  TraceValue("%u",decision_level_properties[curr_level].ply);
  TraceValue("%u",decision_level_properties[curr_level].purpose);
  TraceValue("%u",decision_level_properties[curr_level].object);
  TraceValue("%u",decision_level_properties[curr_level].id);
  TraceEnumerator(Side,decision_level_properties[curr_level].side);
  TraceValue("%u",try_to_avoid_insertion[White]);
  TraceValue("%u",try_to_avoid_insertion[Black]);
  TraceEOL();

  assert(decision_level_properties[curr_level].ply!=0);

  switch (decision_level_properties[curr_level].purpose)
  {
    case decision_purpose_random_mover_backward:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].side==side_failure)
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
          {
            /* try harder.
             * a future decision may
             * - select a better walk
             */
          }
          else
            skip = true;
        }
        else
          skip = true;
      }
      else
        skip = true;
      break;

    case decision_purpose_random_mover_forward:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_departure
          || decision_level_properties[curr_level].object==decision_object_walk
          || decision_level_properties[curr_level].object==decision_object_arrival)
      {
        if (decision_level_properties[curr_level].side==side_failure)
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
          {
            /* try harder.
             * a future decision may
             * - select a better walk
             * - select a better arrival square
             * - select a mover that can't eventually do the capture
             */
          }
          else
            skip = true;
        }
        else
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
          {
            /* try harder.
             * a future decision may
             * - avoid capturing a viable capturer
             */
          }
          else
            skip = true;
        }
      }
      else if (decision_level_properties[curr_level].object==decision_object_random_move)
      {
        if (decision_level_properties[curr_level].side==side_failure)
          skip = true;
        else
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
          {
            /* try harder.
             * a future decision may
             * - avoid capturing a viable capturer
             */
          }
          else
            skip = true;
        }
      }
      else
        assert(0);
      break;

    case decision_purpose_invisible_capturer_existing:
    case decision_purpose_invisible_capturer_inserted:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].side==side_failure)
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
          {
            /* try harder.
             * a future decision may
             * - select a better walk
             */
          }
          else
            skip = true;
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_departure)
      {
        if (decision_level_properties[curr_level].side==side_failure)
        {
          if (decision_level_properties[curr_level].ply<=ply_failure)
            skip = true;
          else
          {
            /* try harder.
             * a future decision may
             * - select a mover that can't eventually do the capture
             */
          }
        }
        else
          skip = true;
      }
      else
        skip = true;
      break;

    case decision_purpose_illegal_check_interceptor:
      if (decision_level_properties[curr_level].object!=decision_object_side
          && decision_level_properties[curr_level].object!=decision_object_placement
          && decision_level_properties[curr_level].object!=decision_object_walk)
      {
        if (decision_level_properties[curr_level].side!=side_failure)
        {
          /* decision concerning the other side can't contribute to being able for this side
           * to capture ...
           * ... but we still have to make sure that this side is examined too!
           */
          skip = true;
        }
      }
      /* placement:
begin
author Ken Kousaka
origin Sake tourney 2018, announcement
pieces TotalInvisible 1 white kb8 qh1 black ka1 sb1e7
stipulation h#2
option movenum start 10:3:7:1
end

             Ken Kousaka
   Sake tourney 2018, announcement

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   K   .   .   .   .   .   .   8
|                                   |
7   .   .   .   .  -S   .   .   .   7
|                                   |
6   .   .   .   .   .   .   .   .   6
|                                   |
5   .   .   .   .   .   .   .   .   5
|                                   |
4   .   .   .   .   .   .   .   .   4
|                                   |
3   .   .   .   .   .   .   .   .   3
|                                   |
2   .   .   .   .   .   .   .   .   2
|                                   |
1  -K  -S   .   .   .   .   .   Q   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  2 + 3 + 1 TI

!test_mate 6:Se7-f5 7:Qh1-a8 8:Sf5-d4 9:TI~-d4 - total_invisible.c:#551 - D:54 - 42
use option start 10:3:7:1 to replay
!  2 + 9 a2 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#510 - D:55
!   3 + 9 b (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#473 - D:57
!    4 9 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#442
!   3 + 9 w (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#473 - D:59
!    4 + 9 P (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#253 - D:61
!     5 8 capture in ply 9 will not be possible - intercept_illegal_checks.c:#73
!    4 + 9 S (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#253 - D:63
!     5 8 capture in ply 9 will not be possible - intercept_illegal_checks.c:#73
!    4 + 9 B (K:0+0 x:0+0 !:0+0 ?:0+0 F:1+0) - intercept_illegal_checks.c:#253 - D:65
!     5 8 capture in ply 9 will not be possible - intercept_illegal_checks.c:#73

HERE!

!  2 + 9 a3 (K:0+0 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#510 - D:67
       */
      /* walk:
begin
author Ofer Comay
origin Sake tourney 2018, 3rd HM, cooked (and 1 author's solution doesn't deliver mate)
pieces TotalInvisible 3 white ke5 qh8 bc1 pb7c2h4 black rb4e1 ba1f1 sf2
stipulation h#2
option movenum start 17:46:23:1
end

             Ofer Comay
Sake tourney 2018, 3rd HM, cooked (and 1 author's solution doesn't deliver mate)

+---a---b---c---d---e---f---g---h---+
|                                   |
8   .   .   .   .   .   .   .   Q   8
|                                   |
7   .   P   .   .   .   .   .   .   7
|                                   |
6   .   .   .   .   .   .   .   .   6
|                                   |
5   .   .   .   .   K   .   .   .   5
|                                   |
4   .  -R   .   .   .   .   .   P   4
|                                   |
3   .   .   .   .   .   .   .   .   3
|                                   |
2   .   .   P   .   .  -S   .   .   2
|                                   |
1  -B   .   B   .  -R  -B   .   .   1
|                                   |
+---a---b---c---d---e---f---g---h---+
  h#2                  6 + 5 + 3 TI

>   1.Bf1-a6 Qh8-c8   2.Rb4-b1 TI~*a6[a6=wS][c3=bK] #

Das schwarze Ding auf der e-Linie kann verstellen!

!test_mate 6:Bf1-a6 7:Qh8-c8 8:Rb4-b1 9:TI~-a6 - total_invisible.c:#551 - D:2060 - 1990
use option start 17:46:23:1 to replay
!  2 + 6 d4 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#510 - D:2061
...
!  2 + 6 c3 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#510 - D:2385
!   3 + 6 w (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#473 - D:2387
...
!   3 + 6 b (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+0) - intercept_illegal_checks.c:#473 - D:2585
!    4 + 6 K (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#253 - D:2587
!     5 + 6 e4 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#510 - D:2589
!      6 + 6 w (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#473 - D:2591
...
!      6 + 6 b (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+1) - intercept_illegal_checks.c:#473 - D:2859
!       7 + 6 P (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#253 - D:2861
!        8 + 9 c4 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#510 - D:2863
...
!        8 + 9 c7 (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#510 - D:2895
!         9 + 9 b (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#473 - D:2897
!          10 9 not enough available invisibles for intercepting all illegal checks - intercept_illegal_checks.c:#442
!         9 + 9 w (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#473 - D:2899
!          10 + 9 P (K:0+1 x:0+0 !:0+0 ?:0+0 F:1+2) - intercept_illegal_checks.c:#253 - D:2901
...
!          10 + 9 B (K:0+1 x:0+0 !:0+0 ?:0+0 F:1+2) - intercept_illegal_checks.c:#253 - D:2911
!           11 8 capture in ply 9 will not be possible - intercept_illegal_checks.c:#73

HERE

!       7 + 6 S (K:0+1 x:0+0 !:0+0 ?:0+0 F:0+2) - intercept_illegal_checks.c:#253 - D:3133
       */
      break;

    default:
      break;
  }

  if (decision_level_properties[curr_level].purpose==decision_purpose_illegal_check_interceptor)
  {
    if (decision_level_properties[curr_level].object==decision_object_side
        || decision_level_properties[curr_level].object==decision_object_walk)
    {
      assert(decision_level_properties[curr_level].side!=no_side);
      if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can insert a victim\n");
#endif

          skip = false;
        }
      }
    }
  }
  else
  {
    assert(decision_level_properties[curr_level].side!=no_side);
    assert(decision_level_properties[curr_level].object!=decision_object_side);
    if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
    {
      if (decision_level_properties[curr_level].object==decision_object_departure
        || decision_level_properties[curr_level].object==decision_object_arrival
        || decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can insert a capturer\n");
#endif

          skip = false;
        }
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",skip);
  TraceFunctionResultEnd();
  return skip;
}

/* Optimise backtracking considering that we have
 * reached a position where we won't able to execute the planned capture by an invisble
 * in the subsequent move because
 * - no existing invisible of the relevant side can reach the capture square
 * - no invisible of the relevant side can be inserted
 * @param side_capturing the side that is supposed to capture
 */
void backtrack_from_failed_capture_by_invisible(Side side_capturing)
{
  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_capturing);
  TraceFunctionParamListEnd();

  TraceValue("%u",nbply);
  TraceEOL();

  current_backtracking = backtrack_failture_to_capture_by_invisible;
  max_decision_level = curr_decision_level-1;

  try_to_avoid_insertion[Black] = false;
  try_to_avoid_insertion[White] = false;
  ply_failure = nbply;
  side_failure = side_capturing;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static boolean failure_to_capture_invisible_by_pawn_continue_level(decision_level_type curr_level)
{
  boolean skip = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",curr_level);
  TraceFunctionParamListEnd();

  TraceValue("%u",curr_level);
  TraceValue("%u",ply_failure);
  TraceEnumerator(Side,side_failure);
  TraceValue("%u",decision_level_properties[curr_level].ply);
  TraceValue("%u",decision_level_properties[curr_level].purpose);
  TraceValue("%u",decision_level_properties[curr_level].object);
  TraceValue("%u",decision_level_properties[curr_level].id);
  TraceEnumerator(Side,decision_level_properties[curr_level].side);
  TraceValue("%u",try_to_avoid_insertion[White]);
  TraceValue("%u",try_to_avoid_insertion[Black]);
  TraceEOL();

  assert(decision_level_properties[curr_level].ply!=0);

  switch (decision_level_properties[curr_level].purpose)
  {
    case decision_purpose_random_mover_backward:
    case decision_purpose_invisible_capturer_inserted:
    case decision_purpose_invisible_capturer_existing:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          /* depending on the walk, this piece may eventually sacrifice itself
           * to allow the capture by pawn
           */
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_departure)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
          skip = true;
        else
        {
          /* we may be able to sacrifice ourselves, either to the capturing pawn or
           * a pawn sacrificing itself to the capturing pawn
           * - by staying where we are (and let another piece move)
           * - by moving away to allow a pawn to sacrifice itself
           */
        }
      }
      else
        skip = true;
      break;

    case decision_purpose_random_mover_forward:
      assert(decision_level_properties[curr_level].side!=no_side);
      if (decision_level_properties[curr_level].object==decision_object_departure
          || decision_level_properties[curr_level].object==decision_object_arrival
          || decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (decision_level_properties[curr_level].ply<ply_failure)
        {
          /* we may be able to sacrifice ourselves, either to the capturing pawn or
           * a pawn sacrificing itself to the capturing pawn
           * - by staying where we are (and let another piece move)
           * - by moving away to allow a pawn to sacrifice itself
           * - by moving to the capture square
           * - by selecting a walk that allows us to eventually move the the capture square
           * - by not accidentally capturing a piece that can eventually sacrifice itself
           */
        }
        else
          skip = true;
      }
      else if (decision_level_properties[curr_level].object==decision_object_random_move)
        skip = true;
      break;

    default:
      break;
  }

  if (decision_level_properties[curr_level].purpose==decision_purpose_illegal_check_interceptor)
  {
    if (decision_level_properties[curr_level].object==decision_object_side
        || decision_level_properties[curr_level].object==decision_object_walk)
    {
      assert(decision_level_properties[curr_level].side!=no_side);
      if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can insert a victim\n");
#endif

          skip = false;
        }
      }
    }
  }
  else
  {
    assert(decision_level_properties[curr_level].side!=no_side);
    assert(decision_level_properties[curr_level].object!=decision_object_side);
    if (try_to_avoid_insertion[decision_level_properties[curr_level].side])
    {
      if (decision_level_properties[curr_level].object==decision_object_departure
        || decision_level_properties[curr_level].object==decision_object_arrival
        || decision_level_properties[curr_level].object==decision_object_walk)
      {
        if (skip)
        {
#if defined(REPORT_DECISIONS)
          printf("!%*s%d ",curr_decision_level,"",curr_decision_level);
          printf("trying to avoid an insertion so that we can insert a victim\n");
#endif

          skip = false;
        }
      }
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",skip);
  TraceFunctionResultEnd();
  return skip;
}

/* Optimise backtracking considering that we have
 * reached a position where we won't able to execute the planned capture of an invisible
 * by a pawn in the subsequent move because
 * - no existing invisible of the relevant side can sacrifice itself on the capture square
 * - no invisible of the relevant side can be inserted
 * @param side_capturing the side that is supposed to capture
 */
void backtrack_from_failed_capture_of_invisible_by_pawn(Side side_capturing)
{
  TraceFunctionEntry(__func__);
  TraceEnumerator(Side,side_capturing);
  TraceFunctionParamListEnd();

  TraceValue("%u",nbply);
  TraceEOL();

  current_backtracking = backtrack_failture_to_capture_invisible_by_pawn;
  max_decision_level = curr_decision_level-1;

  try_to_avoid_insertion[Black] = false;
  try_to_avoid_insertion[White] = false;
  ply_failure = nbply;
  side_failure = advers(side_capturing);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Reduce max_decision_level to a value as low as possible considering that we have
 * determined that the we are done testing the current move sequence
 */
void backtrack_definitively(void)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  current_backtracking = backtrack_until_level;
  max_decision_level = decision_level_forever;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* To be invoked after backtrack_definitively(), possibly multiple times, to make that "definititively"
 * a bit more relative.
 * @param level level to which to backtrack at most
 */
void backtrack_no_further_than(decision_level_type level)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",level);
  TraceFunctionParamListEnd();

  assert(level!=decision_level_uninitialised);
  current_backtracking = backtrack_until_level;

  if (level>max_decision_level)
    max_decision_level = level;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

boolean can_decision_level_be_continued(void)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParamListEnd();

  TraceValue("%u",current_backtracking);
  TraceValue("%u",curr_decision_level);
  TraceValue("%u",max_decision_level);
  TraceEOL();

  switch (current_backtracking)
  {
    case backtrack_none:
      assert(max_decision_level==decision_level_latest);
      result = true;
      break;

    case backtrack_until_level:
      assert(max_decision_level<decision_level_latest);
      result = curr_decision_level<=max_decision_level;
      break;

    case backtrack_failure_to_intercept_illegal_checks_by_invisible:
      assert(max_decision_level<decision_level_latest);
      result = !failure_to_intercept_illegal_checks_continue_level(curr_decision_level);
      break;

    case backtrack_failure_to_intercept_illegal_checks_by_visible:
      assert(max_decision_level<decision_level_latest);
      // TODO do we need separate handling for checks by visibles?
      result = !failure_to_intercept_illegal_checks_continue_level(curr_decision_level);
      break;

    case backtrack_failture_to_capture_by_invisible:
      assert(max_decision_level<decision_level_latest);
      result = !failure_to_capture_by_invisible_continue_level(curr_decision_level);
      break;

    case backtrack_failture_to_capture_invisible_by_pawn:
      assert(max_decision_level<decision_level_latest);
      result = !failure_to_capture_invisible_by_pawn_continue_level(curr_decision_level);
      break;

    default:
      assert(0);
      result = true;
      break;
  };

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}
