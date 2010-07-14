#include "stipulation/series_play/branch.h"
#include "pyslice.h"
#include "pyselfcg.h"
#include "pymovein.h"
#include "stipulation/series_play/play.h"
#include "pypipe.h"
#include "stipulation/branch.h"
#include "stipulation/proxy.h"
#include "stipulation/series_play/fork.h"
#include "stipulation/series_play/move.h"
#include "stipulation/series_play/shortcut.h"
#include "trace.h"

#include <assert.h>

/* Shorten a series pipe by a half-move
 * @param pipe identifies pipe to be shortened
 */
void shorten_series_pipe(slice_index pipe)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",pipe);
  TraceFunctionParamListEnd();

  --slices[pipe].u.branch.length;
  if (slices[pipe].u.branch.min_length>slack_length_series)
    --slices[pipe].u.branch.min_length;

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Allocate a series branch where the next slice's starter is the
 * opponent of the series's starter.
 * @param length maximum number of half-moves of slice (+ slack)
 * @param min_length minimum number of half-moves of slice (+ slack)
 * @param proxy_to_goal identifies proxy slice leading towards goal
 * @return index of entry slice into allocated series branch
 */
slice_index alloc_series_branch(stip_length_type length,
                                stip_length_type min_length,
                                slice_index proxy_to_goal)
{
  slice_index result;
  slice_index const to_goal = slices[proxy_to_goal].u.pipe.next;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",length);
  TraceFunctionParam("%u",min_length);
  TraceFunctionParam("%u",proxy_to_goal);
  TraceFunctionParamListEnd();

  assert(length>slack_length_series);
  assert(slices[proxy_to_goal].type==STProxy);
  assert(to_goal!=no_slice);

  {
    slice_index const
        guard1 = alloc_selfcheck_guard_series_filter(length,min_length);
    slice_index const proxy = alloc_proxy_slice();
    slice_index const move = alloc_series_move_slice(length,min_length);
    slice_index const fork = alloc_series_fork_slice(length,min_length,
                                                     proxy_to_goal);
    slice_index const
        guard2 = alloc_selfcheck_guard_series_filter(length,min_length);
    slice_index const inverter = alloc_move_inverter_series_filter();

    result = alloc_proxy_slice();

    shorten_series_pipe(fork);
    shorten_series_pipe(guard2);

    pipe_link(result,guard1);
    pipe_link(guard1,proxy);
    pipe_link(proxy,move);
    pipe_link(move,fork);
    pipe_link(fork,guard2);
    pipe_link(guard2,inverter);
    pipe_link(inverter,result);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}
