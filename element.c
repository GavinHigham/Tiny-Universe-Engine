#include <string.h>
#include <assert.h>
#include "element.h"
#include "macros.h"
#include "math/utility.h"

struct element_properties all_element_properties[] = {
	{{203, 123, 78}}, //Default element if there are no others defined.
	{{77, 110, 159}},
	{{96, 106, 87}},
	{{145, 60, 43}},
	{{255, 10, 20}},
	{{218, 219, 212}},
	{{168, 107, 83}},
	{{158, 99, 83}},
	{{167, 183, 234}},
	{{24, 30, 130}},
	{{98, 9, 212}},
	{{94, 147, 110}},
};
size_t num_elements = LENGTH(all_element_properties);

struct element_properties element_get_properties(int i)
{
	assert(i < num_elements);
	return all_element_properties[i];	
}

void element_set_all_properties(struct element_properties *props, size_t num_props)
{
	assert(num_props < num_elements);
	for (int i = 0; i < num_props; i++)
		all_element_properties[i] = props[i];
	num_elements = num_props;
}

void element_get_random_set(int *elements, int num)
{
	assert(num <= num_elements);
	int all_elements[num_elements];
	for (int i = 0; i < num_elements; i++)
		all_elements[i] = i;
	int_shuffle(all_elements, num_elements);
	memcpy(elements, all_elements, num*sizeof(int));
}