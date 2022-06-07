Data structures
===============

The logical structure is equivalent to the view in the TreeEditor:
This is the tree of branches and images.

The graphical structure holds different and more information, for
example a branch also has a flagrow, which in turn has a set of flags
and so on.

The logical part is mostly about maintaining and inserting branches and images,
while the graphical is required to maintain the visualization and
calculation of positions and bounding boxes.

I have decided to maintain both structures indepently, which allows
to use the QAbstractItem for the TreeEditor and general maintenance of
the "logical" tree and at the same time use the parent-child relations
of QGraphicsItems for the more complex graphical layouts, like rotated
elements and also to simplify the positioning algorithms. Actually different
map layouts become only possible by decoupling the logical and the
graphical representations.

This approach has pros and cons, of course. For example when deleting a
subtree, both logical and graphical structures need to be considered
differently, which is a bit complicate: The graphical tree get's
decoupled and deleted first, then rest of the logical tree is deleted.

Also for relinking parts to new destinations, the structures need
different considerations.

On the other hand, if all would be combined in one data structure, a lot
of filtering and special cases would be required to support both the
logical operations and also the visualization in different editors, with
different levels of details.


Containers
----------

- Container either represents a graphical object, e.g. the
  HeadingContainer is the text of a branch on the map. Or the container
  has contains (sic!) a set of other containers

- Containers containing others use ContainerLayouts and hints for
  aligning ther subcontainers. This information is used for
  repositioning and defined in the MapDesign

- Containers are no longer part of a BranchObj, but become their own
  structures. In the end they will probably replace the current MapObj and
  inherited classes.
  
- Container classes (like MapObjs previously) don't follow the rule of
  three: This works, because we
    - we only use pointers, esp. for passing to functions 
    - are very care careful to delete containers
    - no containers created on stack
   
Ideas 
-----

* Add/remove containers as needed. Examples
  - Flags, frames, etc. only if set
  - Additional info likes statistics (no. of children, ...)

* Dedicated containers for links
  - Allows more complex links, e.g. curved "bottomlines" under headings,
    maybe curved headings later
  - Proposition to have names on links, maybe rotated to save screen space


Bugs
----

* Position hint for relinking often way to low, not really below new
  parent
  
Next steps
----------

* Moving containers around
    - Todo
        - Fix z value of select box for selected containers
        - absolute position for MapCenters for COntainers

    - moving and tmp linked:
      - maybe also information about rotation of tmp child

      The tmpParentContainer needs to be positioned based on link point and
      orientation from above, also considering modifier keys to link
      above/below. The bounding rect information and also parent needs to
      remain unchanged, to avoid flickering in the tree, but the (relative)
      position itself needs to be adjusted => a new layout hint is required.

    - Animate moving containers when starting to move

      Containers could "gather" below the container, which is under
      mouse pointer, when movement starts (is this the last
      selected one?)

* Cleanup MapObj related stuff, which has been replaced by Containers
    - MapItem
        - Probably pretty much useless once Containers are fully
          implemented. Especially the getEditPosition and
          getSelectionPath should move to containers
    - MapObj
        - need at all in the end?
        - getSelectionPath

* Let containers inherit QGraphicsItem and use a QRectF for geometry instead of inheriting QGraphicsRectItem        
  once drawing boxes is no longer required for debugging
