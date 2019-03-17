/*
For each component that should have its arrays generated, but not its entity_make_<component name> function,
or the code that deletes the component.

This would usually be for components that need some kind of initialization. The "make" and "unmake" function
definitions should call their "alloc" and "dealloc" counterparts.

#define the macro ENTITY_COMPONENT_GEN, include this file, and then #undef it.
Component name capitalized, component name uncapitalized, component buffer count.
*/
ENTITY_COMPONENT_GEN(Drawable,     drawable,     128)
ENTITY_COMPONENT_GEN(Breadcrumber, breadcrumber, 128)