#include "conditions/ohneschach/immobility_tester.h"
#include "pydata.h"
#include "stipulation/stipulation.h"
#include "stipulation/proxy.h"
#include "stipulation/branch.h"
#include "stipulation/boolean/and.h"
#include "debugging/trace.h"

/* This module provides functionality dealing with slices that detect
 * whether a side is immobile
 */

static void optimise_guard(slice_index si, stip_structure_traversal *st)
{
  stip_deep_copies_type * const copies = st->param;

  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  (*copies)[si] = alloc_pipe(STOhneschachCheckGuard);

  stip_traverse_structure_children_pipe(si,st);

  if (slices[si].next1!=no_slice)
    link_to_branch((*copies)[si],(*copies)[slices[si].next1]);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

static void insert_optimiser(slice_index si, stip_structure_traversal *st)
{
  TraceFunctionEntry(__func__);
  TraceFunctionParam("%u",si);
  TraceFunctionParamListEnd();

  {
    slice_index const proxy_nonchecking = alloc_proxy_slice();
    slice_index const proxy_any = alloc_proxy_slice();
    slice_index const and = alloc_and_slice(proxy_nonchecking,proxy_any);

    stip_structure_traversal st_nested;
    stip_deep_copies_type copies;
    init_deep_copy(&st_nested,st,&copies);
    stip_structure_traversal_override_single(&st_nested,
                                             STOhneschachCheckGuardDefense,
                                             &optimise_guard);
    stip_traverse_structure(si,&st_nested);

    pipe_link(slices[si].prev,and);
    pipe_link(proxy_nonchecking,copies[si]);
    pipe_link(proxy_any,si);

    stip_traverse_structure_children_pipe(copies[si],st);
    stip_traverse_structure_children_pipe(si,st);
  }

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}

/* Optimise Ohneschach immobility tester slices
 * @param si where to start (entry slice into stipulation)
 */
void ohneschach_optimise_immobility_testers(slice_index si)
{
  stip_structure_traversal st;

  TraceFunctionEntry(__func__);
  TraceValue("%u",si);
  TraceFunctionParamListEnd();

  stip_structure_traversal_init(&st,0);
  stip_structure_traversal_override_single(&st,
                                           STImmobilityTester,
                                           &insert_optimiser);
  stip_traverse_structure(si,&st);

  TraceFunctionExit(__func__);
  TraceFunctionResultEnd();
}
