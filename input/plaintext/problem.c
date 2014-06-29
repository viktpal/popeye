#include "input/plaintext/problem.h"
#include "input/plaintext/token.h"
#include "input/plaintext/condition.h"
#include "input/plaintext/option.h"
#include "input/plaintext/twin.h"
#include "output/plaintext/protocol.h"
#include "output/plaintext/message.h"
#include "output/plaintext/language_dependant.h"
#include "options/maxsolutions/maxsolutions.h"
#include "optimisations/intelligent/limit_nr_solutions_per_target.h"
#include "options/stoponshortsolutions/stoponshortsolutions.h"
#include "pieces/walks/hunters.h"
#include "solving/move_generator.h"
#include "platform/maxtime.h"
#include "debugging/assert.h"

char ActAuthor[256];
char ActOrigin[256];
char ActTitle[256];
char ActAward[256];
char ActStip[37];

static void InitMetaData(void)
{
  ActTitle[0] = '\0';
  ActAuthor[0] = '\0';
  ActOrigin[0] = '\0';
  ActAward[0] = '\0';
  ActStip[0] = '\0';
}

static void InitBoard(void)
{
  square i;
  square const *bnp;

  for (i= maxsquare-1; i>=0; i--)
  {
    empty_square(i);
    block_square(i);
  }

  /* dummy squares for various purposes -- must be empty */
  empty_square(pawn_multistep);
  empty_square(messigny_exchange);
  empty_square(kingside_castling);
  empty_square(queenside_castling);
  empty_square(retro_capture_departure);

  for (bnp = boardnum; *bnp; bnp++)
    empty_square(*bnp);

  being_solved.king_square[White] = initsquare;
  being_solved.king_square[Black] = initsquare;
}

/* iterate until we detect an input token that identifies the user's language
 * @return the detected language
 */
static Language detect_user_language(void)
{
  while (true)
  {
    char *tok = ReadNextTokStr();

    Language lang;
    for (lang = 0; lang<LanguageCount; ++lang)
      if (GetUniqIndex(TokenCount,TokenString[lang],tok)==BeginProblem)
        return lang;

    output_plaintext_input_error_message(NoBegOfProblem, 0);
  }

  return LanguageCount; /* avoid compiler warning */
}

static void write_problem_footer(void)
{
  if (max_solutions_reached()
      || was_max_nr_solutions_per_target_position_reached()
      || has_short_solution_been_found_in_problem()
      || hasMaxtimeElapsed())
    output_plaintext_message(InterMessage);
  else
    output_plaintext_message(FinishProblem);

  output_plaintext_print_time(" ","");
  output_plaintext_message(NewLine);
  output_plaintext_message(NewLine);
  output_plaintext_message(NewLine);
  protocol_fflush(stdout);
}

/* Iterate over the problems read from standard input or the input
 * file indicated in the command line options
 */
void iterate_problems(void)
{
  Token prev_token;

  UserLanguage = detect_user_language();

  output_plaintext_select_language(UserLanguage);
  output_message_initialise_language(UserLanguage);

  do
  {
    nextply(no_side);
    assert(nbply==ply_diagram_setup);

    InitMetaData();
    InitBoard();
    InitCond();
    InitOpt();

    hunters_reset();

    prev_token = iterate_twins();

    write_problem_footer();

    reset_max_solutions();
    reset_was_max_nr_solutions_per_target_position_reached();
    reset_short_solution_found_in_problem();

    undo_move_effects();
    finply();
  } while (prev_token==NextProblem);
}
