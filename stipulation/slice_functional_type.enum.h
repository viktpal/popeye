typedef enum
{
 slice_function_unspecified, slice_function_proxy, slice_function_move_generator, slice_function_move_reordering_optimiser, slice_function_move_removing_optimiser, slice_function_binary, slice_function_testing_pipe, slice_function_conditional_pipe, slice_function_end_of_branch, slice_function_writer, nr_slice_functional_types,
} slice_functional_type;
extern char const *slice_functional_type_names[];
