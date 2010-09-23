#include "pyrecipr.h"
#include "pyslice.h"
#include "pypipe.h"
#include "pyproc.h"
#include "trace.h"

#include <assert.h>


/* Allocate a reciprocal slice.
 * @param proxy1 proxy to 1st operand
 * @param proxy2 proxy to 2nd operand
 * @return index of allocated slice
 */
slice_index alloc_reciprocal_slice(slice_index proxy1, slice_index proxy2)
{
  slice_index result;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",proxy1);
  TraceFunctionParam("%u",proxy2);
  TraceFunctionParamListEnd();

  assert(proxy1!=no_slice);
  assert(slices[proxy1].type==STProxy);
  assert(proxy2!=no_slice);
  assert(slices[proxy2].type==STProxy);

  result = alloc_slice(STReciprocal);

  slices[result].u.binary.op1 = proxy1;
  slices[result].u.binary.op2 = proxy2;

  TraceFunctionExit(__func__);
  TraceFunctionResult("%u",result);
  TraceFunctionResultEnd();
  return result;
}

/* Determine whether there is a solution at the end of a quodlibet
 * slice.
 * @param si slice index
 * @return whether there is a solution and (to some extent) why not
 */
has_solution_type reci_has_solution(slice_index si)
{
  slice_index const op1 = slices[si].u.binary.op1;
  slice_index const op2 = slices[si].u.binary.op2;
  has_solution_type result;
  has_solution_type result1;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  result1 = slice_has_solution(op1);
  if (result1==opponent_self_check)
    result = opponent_self_check;
  else
  {
    has_solution_type const result2 = slice_has_solution(op2);
    result = result1>result2 ? result2 : result1;
  }

  TraceFunctionExit(__func__);
  TraceEnumerator(has_solution_type,result,"");
  TraceFunctionParamListEnd();
  return result;
}

/* Solve a slice
 * @param si slice index
 * @return whether there is a solution and (to some extent) why not
 */
has_solution_type reci_solve(slice_index si)
{
  has_solution_type result;
  slice_index const op1 = slices[si].u.binary.op1;
  slice_index const op2 = slices[si].u.binary.op2;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  TraceValue("%u",op1);
  TraceValue("%u\n",op2);

  switch (slice_has_solution(op2))
  {
    case has_solution:
      result = slice_solve(op1);
      if (result==has_solution)
      {
        has_solution_type const result2 = slice_solve(op2);
        assert(result2==has_solution);
      }
      break;

    default:
      result = has_no_solution;
      break;
  }

  TraceFunctionExit(__func__);
  TraceEnumerator(has_solution_type,result,"");
  TraceFunctionResultEnd();
  return result;
}

/* Detect starter field with the starting side if possible.
 * @param si identifies slice being traversed
 * @param st status of traversal
 */
void reci_detect_starter(slice_index si, stip_structure_traversal *st)
{
  slice_index const op1 = slices[si].u.binary.op1;
  slice_index const op2 = slices[si].u.binary.op2;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  if (slices[si].starter==no_side)
  {
    stip_traverse_structure(op1,st);
    stip_traverse_structure(op2,st);

    if (slices[op1].starter==no_side)
      slices[si].starter = slices[op2].starter;
    else
    {
      assert(slices[op1].starter==slices[op2].starter
             || slices[op2].starter==no_side);
      slices[si].starter = slices[op1].starter;
    }
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
