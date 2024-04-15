## Blender

Scenes are created in blender. It is far from optimal as a level editor but it gets the job done.
The folder `assets/library` should be registered in blender as an asset library.

## Names
Names should use `PascalCase` and may be separated with a dot `.`.
Example: `Checkpoint.Ring.Mesh`

## Physics Nodes
Nodes whose names start with `Phys.` will be loaded as Jolt physics bodies.
A physics node must have a collider.
For simple shapes an empty node is used and the shape is specified in the name:
- `Box`  
  Creates a box with side lengths equal to the node's scale.
- `Cylinder`  
  Creates a vertical cylinder with it's height equal to the two times the node's y-scale.  
  The radius is the maximum of the x- or z-scale.
- `Sphere`  
  Creates a sphere with it's radius equal to the maximum of the node's x-, y- or z-scale.

Complex shapes can be specified by simply using a mesh node.

## Custom Properties

### entity
Specifies the name of an entity class.
Nodes with this property will have the specified class instantiated on load.

### tags
A comma seperated list of tags. A tag can be any string without spaces. 
Leading and trailing whitespace is stripped from each tag.

### trigger
Specifies the trigger action name.
Nodes with this property will be created as Jolt sensors.
Has no effect on non-physics nodes.

### prop.*
Custom properties starting with `prop.` will be stored in a key-value map.
The key is equal to the property name without the `prop.` prefix.
The value can be a boolean, float, integer, string or an object reference.
Object references will be converted to string properties containing the (unique) name of the refereced node.