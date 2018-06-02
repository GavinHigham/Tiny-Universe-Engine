/*
#define the macro ENTITY_COMPONENT_DECLARE, include this file, and then #undef it.
Component name capitalized, component name uncapitalized, component buffer count.
*/
ENTITY_COMPONENT_DECLARE(Drawable,     drawable,     128)
ENTITY_COMPONENT_DECLARE(Controllable, controllable, 128)
ENTITY_COMPONENT_DECLARE(Scriptable,   scriptable,   128)
ENTITY_COMPONENT_DECLARE(Physical,     physical,     128)
ENTITY_COMPONENT_DECLARE(Emissive,     emissive,     128)

/*
ENTITY_COMPONENT_DECLARE(Collidable,   collidable,   128)
Occludable
Occludent
Parent_transform? Could create a chain of transforms.
*/