/*
For each component that should have its arrays and entity_make_<component name> function generated.

#define the macro ENTITY_COMPONENT_GEN, include this file, and then #undef it.
Component name capitalized, component name uncapitalized, component buffer count.
*/
ENTITY_COMPONENT_GEN(Controllable, controllable, 128)
ENTITY_COMPONENT_GEN(Scriptable,   scriptable,   128)
ENTITY_COMPONENT_GEN(Physical,     physical,     128)
// ENTITY_COMPONENT_GEN(Parented,     parented,     128)
// ENTITY_COMPONENT_GEN(Emissive,     emissive,     128)
// ENTITY_COMPONENT_GEN(Mesh,         mesh,         128)

/*
ENTITY_COMPONENT_GEN(Collidable,   collidable,   128)
Occludable
Occludent
Parent_transform? Could create a chain of transforms.
*/