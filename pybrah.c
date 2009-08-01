#include "pybrah.h"
#include "pydata.h"
#include "pyproc.h"
#include "pyint.h"
#include "pymsg.h"
#include "pyoutput.h"
#include "pyslice.h"
#include "pyhelp.h"
#include "pyhelpha.h"
#include "pybrafrk.h"
#include "pyreflxg.h"
#include "trace.h"
#include "platform/maxtime.h"

#include <assert.h>

/* Allocate a STBranchHelp slice.
 * @param next identifies next slice
 * @return index of allocated slice
 */
slice_index alloc_branch_h_slice(stip_length_type length,
                                 stip_length_type min_length,
                                 slice_index next)
{
  slice_index const result = alloc_slice_index();

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  slices[result].type = STBranchHelp; 
  slices[result].starter = no_side; 
  slices[result].u.pipe.next = next;
  slices[result].u.pipe.u.branch.length = length;
  slices[result].u.pipe.u.branch.min_length = min_length;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Impose the starting side on a stipulation
 * @param si identifies branch
 * @param st address of structure that holds the state of the traversal
 * @return true iff the operation is successful in the subtree of
 *         which si is the root
 */
boolean branch_h_impose_starter(slice_index si, slice_traversal *st)
{
  boolean const result = true;
  Side * const starter = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",*starter);
  TraceFunctionParamListEnd();

  slices[si].starter = *starter;

  *starter = advers(*starter);
  slice_traverse_children(si,st);
  *starter = slices[si].starter;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether a side has reached the goal
 * @param just_moved side that has just moved
 * @param si slice index
 * @return true iff just_moved has reached the goal
 */
boolean branch_h_is_goal_reached(Side just_moved, slice_index si)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_is_goal_reached(just_moved,slices[si].u.pipe.next);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine and write the solution(s) in a help stipulation
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 *          (this may be shorter than the slice's length if we are
 *          searching for short solutions only)
 * @return true iff >= 1 solution has been found
 */
boolean branch_h_solve_in_n(slice_index si, stip_length_type n)

{
  boolean result = false;
  Side const side_at_move = slices[si].starter;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_help);

  active_slice[nbply+1] = si;
  genmove(side_at_move);
  
  --MovesLeft[side_at_move];

  while (encore())
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && help_solve_in_n(slices[si].u.pipe.next,n-1))
      result = true;

    repcoup();

    /* Stop solving if a given number of solutions was encountered */
    if (OptFlag[maxsols] && solutions>=maxsolutions)
    {
      TraceValue("%u",maxsolutions);
      TraceValue("%u",solutions);
      TraceText("aborting\n");
      break;
    }

    if (periods_counter>=nr_periods)
      break;
  }
    
  ++MovesLeft[side_at_move];

  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine and write solution(s): add first moves to table (as
 * threats for the parent slice.
 * @param continuations table where to add first moves
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 */
void branch_h_solve_continuations_in_n(table continuations,
                                       slice_index si,
                                       stip_length_type n)
{
  Side const side_at_move = slices[si].starter;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>slack_length_help);

  active_slice[nbply+1] = si;
  genmove(side_at_move);
  
  --MovesLeft[side_at_move];

  while (encore())
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && help_solve_in_n(slices[si].u.pipe.next,n-1))
    {
      append_to_top_table();
      coupfort();
    }

    repcoup();

    /* Stop solving if a given number of solutions was encountered */
    if (OptFlag[maxsols] && solutions>=maxsolutions)
    {
      TraceValue("%u",maxsolutions);
      TraceValue("%u",solutions);
      TraceText("aborting\n");
      break;
    }

    if (periods_counter>=nr_periods)
      break;
  }
    
  ++MovesLeft[side_at_move];

  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Determine whether the slice has a solution in n half moves.
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 * @return true iff >= 1 solution has been found
 */
boolean branch_h_has_solution_in_n(slice_index si, stip_length_type n)
{
  Side const side_at_move = slices[si].starter;
  boolean result = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  active_slice[nbply+1] = si;
  genmove(side_at_move);
  
  --MovesLeft[side_at_move];

  while (encore() && !result)
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && help_has_solution_in_n(slices[si].u.pipe.next,n-1))
      result = true;

    repcoup();
  }
    
  ++MovesLeft[side_at_move];

  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/** help adapter *****************************************************/

/* Allocate a STHelpAdapter slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of allocated slice
 */
slice_index alloc_help_adapter_slice(stip_length_type length,
                                     stip_length_type min_length,
                                     slice_index fork,
                                     slice_index next)
{
  slice_index const result = alloc_slice_index();

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  slices[result].type = STHelpAdapter; 
  slices[result].starter = no_side; 
  slices[result].u.pipe.next = next;
  slices[result].u.pipe.u.help_adapter.length = length;
  slices[result].u.pipe.u.help_adapter.min_length = min_length;
  slices[result].u.pipe.u.help_adapter.fork = fork;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Promote a slice that was created as STHelpAdapter to STHelpRoot
 * because the assumption that the slice is nested in some other slice
 * turned out to be wrong.
 * @param adapter identifies slice to be promoted
 */
void help_adapter_promote_to_toplevel(slice_index adapter)
{
  slice_index const branch = slices[adapter].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",adapter);
  TraceFunctionParamListEnd();

  assert(slices[adapter].type==STHelpAdapter);
  assert(slices[adapter].u.pipe.u.help_adapter.length-slack_length_help==1);
  assert(slices[branch].type==STBranchHelp);

  slices[adapter].type = STHelpRoot;
  slices[adapter].u.pipe.next = slices[branch].u.pipe.next;
  dealloc_slice_index(branch);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Solve a branch slice at non-root level.
 * @param si slice index
 * @return true iff >=1 solution was found
 */
boolean help_adapter_solve(slice_index si)
{
  boolean result = false;
  slice_index const next = slices[si].u.pipe.next;
  stip_length_type const full_length = slices[si].u.pipe.u.help_adapter.length;
  stip_length_type len = slices[si].u.pipe.u.help_adapter.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(full_length>slack_length_help);

  while (len<full_length && !result)
  {
    if (help_solve_in_n(next,len))
    {
      result = true;
      FlagShortSolsReached = true;
    }

    len += 2;
  }

  result = result || help_solve_in_n(next,full_length);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Find and write post key play
 * @param si slice index
 */
void help_adapter_solve_postkey(slice_index si)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(slices[si].u.pipe.u.help_adapter.length==slack_length_help+1);
  slice_solve_postkey(slices[si].u.pipe.u.help_adapter.fork);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Determine and write continuations of a slice
 * @param continuations table where to store continuing moves (i.e. threats)
 * @param si index of branch slice
 */
void help_adapter_solve_continuations(table continuations, slice_index si)
{
  boolean solution_found = false;
  stip_length_type const full_length = slices[si].u.pipe.u.help_adapter.length;
  stip_length_type len = slices[si].u.pipe.u.help_adapter.min_length;
  slice_index const next = slices[si].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(full_length>=slack_length_help);

  while (len<full_length && !solution_found)
  {
    help_solve_continuations_in_n(continuations,next,len);
    if (table_length(continuations)>0)
    {
      solution_found = true;
      FlagShortSolsReached = true;
    }

    len += 2;
  }

  if (!solution_found)
    help_solve_continuations_in_n(continuations,next,full_length);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Write the key just played
 * @param si slice index
 * @param type type of attack
 */
void help_adapter_root_write_key(slice_index si, attack_type type)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",type);
  TraceFunctionParamListEnd();

  write_attack(type);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Write a priori unsolvability (if any) of a slice (e.g. forced
 * reflex mates).
 * Assumes slice_must_starter_resign(si)
 * @param si slice index
 */
void help_adapter_write_unsolvability(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(slices[fork].type==STBranchFork);
  slice_write_unsolvability(to_goal);
  
  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Is there no chance left for the starting side at the move to win?
 * E.g. did the defender just capture that attacker's last potential
 * mating piece?
 * @param si slice index
 * @return true iff starter must resign
 */
boolean help_adapter_must_starter_resign(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(slices[fork].type==STBranchFork);
  result = slice_must_starter_resign(to_goal);
  
  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether a branch slice.has just been solved with the
 * just played move by the non-starter
 * @param si slice identifier
 * @return true iff the non-starting side has just solved
 */
boolean help_adapter_has_non_starter_solved(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_has_non_starter_solved(to_goal);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the starting side has made such a bad move that
 * it is clear without playing further that it is not going to win.
 * E.g. in s# or r#, has it taken the last potential mating piece of
 * the defender?
 * @param si slice identifier
 * @return true iff starter has lost
 */
boolean help_adapter_has_starter_apriori_lost(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_has_starter_apriori_lost(to_goal);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the attacker has won with his move just played
 * @param si slice identifier
 * @return true iff the starter has won
 */
boolean help_adapter_has_starter_won(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_has_starter_won(to_goal);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether the attacker has reached slice si's goal with his
 * move just played.
 * @param si slice identifier
 * @return true iff the starter reached the goal
 */
boolean help_adapter_has_starter_reached_goal(slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_has_starter_reached_goal(to_goal);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether a side has reached the goal
 * @param just_moved side that has just moved
 * @param si slice index
 * @return true iff just_moved has reached the goal
 */
boolean help_adapter_is_goal_reached(Side just_moved, slice_index si)
{
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
  slice_index const to_goal = slices[fork].u.pipe.u.branch_fork.towards_goal;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = slice_is_goal_reached(just_moved,to_goal);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether a slice has a solution
 * @param si slice index
 * @return true iff slice si has a solution
 */
boolean help_adapter_has_solution(slice_index si)
{
  boolean result = false;
  stip_length_type const full_length = slices[si].u.pipe.u.help_adapter.length;
  stip_length_type len = slices[si].u.pipe.u.help_adapter.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(full_length>=slack_length_help);

  while (len<=full_length)
    if (help_has_solution_in_n(slices[si].u.pipe.next,len))
    {
      result = true;
      break;
    }
    else
      len += 2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean find_relevant_slice_branch_fork(slice_index si,
                                               slice_traversal *st)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result = traverse_slices(slices[si].u.pipe.u.branch_fork.towards_goal,st);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static boolean find_relevant_slice_found(slice_index si, slice_traversal *st)
{
  boolean const result = true;
  slice_index * const to_be_found = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  *to_be_found = si;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

static slice_operation const relevant_slice_finders[] =
{
  &find_relevant_slice_found,         /* STBranchDirect */
  0,                                  /* STBranchDirectDefender */
  &find_relevant_slice_found,         /* STBranchHelp */
  &find_relevant_slice_found,         /* STBranchSeries */
  &find_relevant_slice_branch_fork,   /* STBranchFork */
  &find_relevant_slice_found,         /* STLeafDirect */
  &find_relevant_slice_found,         /* STLeafHelp */
  &find_relevant_slice_found,         /* STLeafSelf */
  &find_relevant_slice_found,         /* STLeafForced */
  &find_relevant_slice_found,         /* STReciprocal */
  &find_relevant_slice_found,         /* STQuodlibet */
  &find_relevant_slice_found,         /* STNot */
  &find_relevant_slice_found,         /* STMoveInverter */
  0,                                  /* STHelpRoot */
  &find_relevant_slice_found,         /* STHelpAdapter */
  &find_relevant_slice_found,         /* STHelpHashed */
  &find_relevant_slice_found,         /* STSelfCheckGuard */
  &find_relevant_slice_found,         /* STReflexGuard */
  0,                                  /* STGoalReachableGuard */
  &find_relevant_slice_found          /* STKeepMatingGuard */
};

/* Detect starter field with the starting side if possible. 
 * @param si identifies slice
 * @param same_side_as_root does si start with the same side as root?
 * @return does the leaf decide on the starter?
 */
who_decides_on_starter help_adapter_detect_starter(slice_index si,
                                                   boolean same_side_as_root)
{
  who_decides_on_starter result = dont_know_who_decides_on_starter;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",same_side_as_root);
  TraceFunctionParamListEnd();

  if (slices[si].starter==no_side)
  {
    boolean const even_length = slices[si].u.pipe.u.help_adapter.length%2==0;
    boolean const fork_same_side_as_root = (even_length
                                            ? same_side_as_root
                                            : !same_side_as_root);
    slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;
    slice_index next_relevant = no_slice;
    slice_traversal st;

    slice_traversal_init(&st,&relevant_slice_finders,&next_relevant);
    traverse_slices(fork,&st);

    result = slice_detect_starter(fork,fork_same_side_as_root);

    TraceValue("%u",next_relevant);
    TraceEnumerator(SliceType,slices[next_relevant].type," ");
    TraceValue("%u\n",even_length);

    switch (slices[next_relevant].type)
    {
      case STLeafDirect:
      {
        if (slices[fork].starter==no_side)
          slices[si].starter = Black;
        else
          slices[si].starter = (even_length
                                ? slices[fork].starter
                                : advers(slices[fork].starter));
        break;
      }

      case STLeafSelf:
      case STLeafHelp:
      {
        if (slices[fork].starter==no_side)
          slices[si].starter = White;
        else
          slices[si].starter = (even_length
                                ? slices[fork].starter
                                : advers(slices[fork].starter));
        break;
      }

      default:
        slices[si].starter = (even_length
                              ? slices[fork].starter
                              : advers(slices[fork].starter));
        break;
    }

  }
  else
    result = leaf_decides_on_starter;

  TraceValue("%u\n",slices[si].starter);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/*************** root *****************/

/* Allocate a STHelpRoot slice.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of allocated slice
 */
slice_index alloc_help_root_slice(stip_length_type length,
                                  stip_length_type min_length,
                                  slice_index fork,
                                  slice_index next)
{
  slice_index const result = alloc_slice_index();

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  slices[result].type = STHelpRoot; 
  slices[result].starter = no_side; 
  slices[result].u.pipe.next = next;
  slices[result].u.pipe.u.help_adapter.length = length;
  slices[result].u.pipe.u.help_adapter.min_length = min_length;
  slices[result].u.pipe.u.help_adapter.fork = fork;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Impose the starting side on a stipulation
 * @param si identifies branch
 * @param st address of structure that holds the state of the traversal
 * @return true iff the operation is successful in the subtree of
 *         which si is the root
 */
boolean help_root_impose_starter(slice_index si, slice_traversal *st)
{
  boolean const result = true;
  Side * const starter = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParam("%u",*starter);
  TraceFunctionParamListEnd();

  slices[si].starter = *starter;

  *starter = advers(*starter);
  slice_traverse_children(si,st);
  *starter = slices[si].starter;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Shorten a help pipe by a half-move
 * @param pipe identifies pipe to be shortened
 */
static void shorten_help_pipe(slice_index pipe)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",pipe);
  TraceFunctionParamListEnd();

  --slices[pipe].u.pipe.u.help_adapter.length;
  --slices[pipe].u.pipe.u.help_adapter.min_length;
  if (slices[pipe].u.pipe.u.help_adapter.min_length<slack_length_help)
    slices[pipe].u.pipe.u.help_adapter.min_length += 2;
  slices[pipe].starter = (slices[pipe].starter==no_side
                          ? no_side
                          : advers(slices[pipe].starter));
  TraceValue("%u",slices[pipe].starter);
  TraceValue("%u",slices[pipe].u.pipe.u.help_adapter.length);
  TraceValue("%u\n",slices[pipe].u.pipe.u.help_adapter.min_length);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Shorten a help branch that is the root of set play. Reduces the
 * length members of slices[root] and resets the next member to the
 * appropriate position.
 * @param root index of the help root slice
 */
static void shorten_setplay_root_branch(slice_index root)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParamListEnd();

  if ((slices[root].u.pipe.u.help_adapter.length-slack_length_help)%2==0)
  {
    slice_index const branch1 = slices[root].u.pipe.next;
    slice_index const fork = slices[branch1].u.pipe.next;
    assert(slices[fork].type==STBranchFork);
    assert(slices[branch1].type==STBranchHelp);
    slices[root].u.pipe.next = fork;
  }
  else
  {
    slice_index const fork = slices[root].u.pipe.next;
    slice_index const branch1 = slices[fork].u.pipe.next;
    slice_index const branch2 = slices[branch1].u.pipe.next;
    assert(slices[fork].type==STBranchFork);
    assert(slices[branch1].type==STBranchHelp);
    assert(slices[branch2].type==STBranchHelp);
    slices[root].u.pipe.next = branch2;
  }

  shorten_help_pipe(root);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Spin off a set play slice at root level
 * @param si slice index
 * @return set play slice spun off; no_slice if not applicable
 */
slice_index help_root_make_setplay_slice(slice_index si)
{
  slice_index result;
  slice_index const fork = slices[si].u.pipe.u.help_adapter.fork;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(slices[si].type==STHelpRoot);

  assert(slices[si].u.pipe.u.help_adapter.length>slack_length_help);

  if (slices[si].u.pipe.u.help_adapter.length==slack_length_help+1)
    result = slices[fork].u.pipe.u.branch_fork.towards_goal;
  else
  {
    result = copy_slice(si);
    shorten_setplay_root_branch(result);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Shorten a root help branch. Reduces the length members of
 * slices[root] and resets the next member to the appropriate
 * position.
 * @param root index of the help root slice
 */
static void shorten_root_branch(slice_index root)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParamListEnd();

  if ((slices[root].u.pipe.u.help_adapter.length-slack_length_help)%2==0)
  {
    slice_index const branch1 = slices[root].u.pipe.next;
    slice_index const fork = slices[branch1].u.pipe.next;
    assert(slices[fork].type==STBranchFork);
    assert(slices[branch1].type==STBranchHelp);
    slices[root].u.pipe.next = fork;
    if (slices[root].u.pipe.u.help_adapter.length==slack_length_help+2)
    {
      assert(slices[fork].u.pipe.next==no_slice);
      dealloc_slice_index(branch1);
    }
  }
  else
  {
    slice_index const fork = slices[root].u.pipe.next;
    slice_index const branch1 = slices[fork].u.pipe.next;
    slice_index const branch2 = slices[branch1].u.pipe.next;
    assert(slices[fork].type==STBranchFork);
    assert(slices[branch1].type==STBranchHelp);
    assert(slices[branch2].type==STBranchHelp);
    slices[root].u.pipe.next = branch2;
    if (slices[root].u.pipe.u.help_adapter.length==slack_length_help+3)
    {
      slices[fork].u.pipe.next = no_slice;
      dealloc_slice_index(branch1);
    }
  }

  shorten_help_pipe(root);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Shorten a help branch by a half-move. If the branch represents a
 * half-move only, deallocates the branch.
 * @param si identifies the branch
 * @return - no_slice if not applicable (already shortened)
 *         - slice representing subsequent play if root has 1 half-move only
 *         - root (shortened) otherwise
 */
slice_index help_root_shorten_help_play(slice_index root)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParamListEnd();

  assert(slices[root].type==STHelpRoot);
  assert(slices[root].u.pipe.u.help_adapter.length>slack_length_help);

  if (slices[root].u.pipe.u.help_adapter.length==slack_length_help+1)
    result = branch_deallocate_to_fork(root);
  else
  {
    shorten_root_branch(root);
    result = root;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve short solutions in exactly n in help play at root level.
 * @param root slice index
 * @param n number of half moves
 * @return true iff >=1 short solution was found
 */
static boolean solve_short_in_n(slice_index root, stip_length_type n)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  if (n==slack_length_help)
  {
    slice_index const fork = slices[root].u.pipe.u.help_adapter.fork;
    result = help_solve_in_n(fork,n);
  }
  else
    /* TODO results are not written to hash table
     */
    result = branch_h_solve_in_n(root,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine and write the solution(s) in a help stipulation
 * @param si slice index of slice being solved
 * @param n number of half moves until end state has to be reached
 *          (this may be shorter than the slice's length if we are
 *          searching for short solutions only)
 * @return true iff >=1 solution was found
 */
static boolean solve_full_in_n(slice_index root, stip_length_type n)
{
  Side const starter = slices[root].starter;
  slice_index const next_slice = slices[root].u.pipe.next;
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",n);
  TraceFunctionParam("%u",root);
  TraceFunctionParamListEnd();

  assert(n>slack_length_help);

  active_slice[nbply+1] = root;
  genmove(starter);
  
  --MovesLeft[starter];

  while (encore())
  {
    if (jouecoup(nbply,first_play) && TraceCurrentMove(nbply)
        && !(OptFlag[restart] && MoveNbr<RestartNbr)
        && help_solve_in_n(next_slice,n-1))
      result = true;

    if (OptFlag[movenbr])
      IncrementMoveNbr();

    repcoup();

    if (OptFlag[maxsols] && solutions>=maxsolutions)
    {
      TraceValue("%u",maxsolutions);
      TraceValue("%u",solutions);
      TraceText("aborting\n");
      break;
    }

    if (periods_counter>=nr_periods)
      break;
  }
    
  ++MovesLeft[starter];

  finply();

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve full-length solutions in exactly n in help play at root level
 * @param root slice index
 * @param n number of half moves
 * @return true iff >=1 solution was found
 */
boolean help_root_solve_in_n(slice_index root, stip_length_type n)
{
  boolean result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParam("%u",n);
  TraceFunctionParamListEnd();

  assert(n>=slack_length_help);

  if (n==slices[root].u.pipe.u.help_adapter.length)
    result = solve_full_in_n(root,n);
  else
    result = solve_short_in_n(root,n);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Solve a branch slice at root level.
 * @param si slice index
 * @return no_slice if set play not applicable
 *         new root slice index (may be equal to old one) otherwise
 * @return true iff >=1 solution was found
 */
boolean help_root_solve(slice_index root)
{
  boolean result = false;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",root);
  TraceFunctionParamListEnd();

  init_output(root);

  if (echecc(nbply,advers(slices[root].starter)))
    ErrorMsg(KingCapture);
  else
  {
    stip_length_type const
        full_length = slices[root].u.pipe.u.help_adapter.length;
    stip_length_type len = (OptFlag[restart]
                            ? full_length
                            : slices[root].u.pipe.u.help_adapter.min_length);

    TraceValue("%u",slices[root].u.pipe.u.help_adapter.min_length);
    TraceValue("%u\n",slices[root].u.pipe.u.help_adapter.length);

    assert(slices[root].u.pipe.u.help_adapter.min_length>=slack_length_help);

    move_generation_mode = move_generation_not_optimized;

    FlagShortSolsReached = false;
    solutions = 0;

    while (len<full_length
           && !(OptFlag[stoponshort] && result))
    {
      if (isIntelligentModeActive)
      {
        if (Intelligent(root,len,full_length))
          result = true;
      }
      else
      {
        if (solve_short_in_n(root,len))
          result = true;
      }

      len += 2;
    }

    if (result && OptFlag[stoponshort])
    {
      TraceText("aborting because of short solutions\n");
      FlagShortSolsReached = true;
    }
    else if (isIntelligentModeActive)
      result = Intelligent(root,full_length,full_length);
    else
      result = solve_full_in_n(root,full_length);

    if (OptFlag[maxsols] && solutions>=maxsolutions)
      /* signal maximal number of solutions reached to outer world */
      FlagMaxSolsReached = true;
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether a slice has a solution
 * @param si slice index
 * @return true iff slice si has a solution
 */
boolean help_root_has_solution(slice_index si)
{
  boolean result = false;
  stip_length_type const full_length = slices[si].u.pipe.u.help_adapter.length;
  stip_length_type len = slices[si].u.pipe.u.help_adapter.min_length;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  assert(full_length>=slack_length_help);

  while (len<=full_length)
    if (branch_h_has_solution_in_n(si,len))
    {
      result = true;
      break;
    }
    else
      len += 2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a top level help branch
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of allocated slice
 */
static slice_index alloc_toplevel_help_branch(stip_length_type length,
                                              stip_length_type min_length,
                                              slice_index next)
{
  slice_index result;
  slice_index fork;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  assert(length>slack_length_help);

  fork = alloc_branch_fork_slice(no_slice,next);

  if (length-slack_length_help==1)
    result = alloc_help_root_slice(length,min_length,fork,fork);
  else if (length-slack_length_help==2)
  {
    slice_index const branch = alloc_branch_h_slice(length,min_length,fork);
    result = alloc_help_root_slice(length,min_length,fork,branch);
  }
  else
  {
    if ((length-slack_length_help)%2==0)
    {
      slice_index const branch1 = alloc_branch_h_slice(length,min_length,
                                                       fork);
      slice_index const branch2 = alloc_branch_h_slice(length-2,min_length,
                                                       branch1);
      shorten_help_pipe(branch1);
      result = alloc_help_root_slice(length,min_length,fork,branch1);

      slices[fork].u.pipe.next = branch2;
    }
    else
    {
      slice_index const branch1 = alloc_branch_h_slice(length-2,min_length,
                                                       fork);
      slice_index const branch2 = alloc_branch_h_slice(length,min_length,
                                                       branch1);
      shorten_help_pipe(branch2);
      result = alloc_help_root_slice(length,min_length,fork,fork);

      slices[fork].u.pipe.next = branch2;
    }

    TraceValue("%u\n",slices[fork].u.pipe.next);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a nested help branch
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of allocated slice
 */
static slice_index alloc_nested_help_branch(stip_length_type length,
                                            stip_length_type min_length,
                                            slice_index next)
{
  slice_index result;
  slice_index fork;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  assert(length>slack_length_help);

  fork = alloc_branch_fork_slice(no_slice,next);

  if (length-slack_length_help==1)
  {
    slice_index const branch = alloc_branch_h_slice(length,min_length,fork);
    result = alloc_help_adapter_slice(length,min_length,fork,branch);
  }
  else
  {
    if (length-slack_length_help==2)
    {
      slice_index const branch1 = alloc_branch_h_slice(length,min_length,
                                                       fork);
      slice_index const branch2 = alloc_branch_h_slice(length,min_length,
                                                       branch1);
      shorten_help_pipe(branch1);
      result = alloc_help_adapter_slice(length,min_length,fork,branch2);
    }
    else
    {
      if ((length-slack_length_help)%2==0)
      {
        slice_index const branch1 = alloc_branch_h_slice(length,min_length,
                                                         fork);
        slice_index const branch2 = alloc_branch_h_slice(length,min_length,
                                                         branch1);
        shorten_help_pipe(branch1);
        result = alloc_help_adapter_slice(length,min_length,fork,branch2);

        slices[fork].u.pipe.next = branch2;
      }
      else
      {
        slice_index const branch1 = alloc_branch_h_slice(length,min_length,
                                                         fork);
        slice_index const branch2 = alloc_branch_h_slice(length,min_length,
                                                         branch1);
        shorten_help_pipe(branch2);
        result = alloc_help_adapter_slice(length,min_length,fork,branch1);

        slices[fork].u.pipe.next = branch2;
      }

      TraceValue("%u\n",slices[fork].u.pipe.next);
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a help branch.
 * @param level is this a top-level branch or one nested into another
 *              branch?
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of adapter slice of allocated help branch
 */
slice_index alloc_help_branch(branch_level level,
                              stip_length_type length,
                              stip_length_type min_length,
                              slice_index next)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",level);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  if (level==toplevel_branch)
    result = alloc_toplevel_help_branch(length,min_length,next);
  else
    result = alloc_nested_help_branch(length,min_length,next);

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Allocate a help branch representing helpreflex play.
 * @param level is this a top-level branch or one nested into another
 *              branch?
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param next identifies next slice
 * @return index of adapter slice of allocated help branch
 */
slice_index alloc_helpreflex_branch(branch_level level,
                                    stip_length_type length,
                                    stip_length_type min_length,
                                    slice_index next)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",level);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",next);
  TraceFunctionParamListEnd();

  assert(level==toplevel_branch);
  assert(level-slack_length_help>1);

  result = alloc_help_branch(level,length,min_length,next);

  if ((length-slack_length_help)%2==0)
  {
    slice_index const tobereplaced = branch_find_slice(STBranchHelp,result);
    assert(tobereplaced!=no_slice);
    insert_reflex_guard_slice(tobereplaced,next);
  }
  else
  {
    slice_index const fork = branch_find_slice(STBranchFork,result);
    slice_index const branch1 = branch_find_slice(STBranchHelp,fork);
    slice_index const tobereplaced = branch_find_slice(STBranchHelp,branch1);
    assert(fork!=no_slice);
    assert(branch1!=no_slice);
    assert(tobereplaced!=no_slice);
    insert_reflex_guard_slice(tobereplaced,next);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}
