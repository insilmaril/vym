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
  aligning their subcontainers. This information is used for
  repositioning and defined in the MapDesign

- Containers are no longer part of a BranchObj, but become their own
  structures. In the end they will probably replace the current MapObj and
  inherited classes.
  
- Container classes (like MapObjs previously) don't follow the rule of
  three: This works, because we
    - we only use pointers, esp. for passing to functions 
    - are very care careful to delete containers
    - no containers created on stack
   
- Container layout
    - BranchContainer
        - [Optional: FrameContainer]
        - InnerContainer
            - OrnamentsContainer
                [Optional/Later: systemFlagsContainer]
                [Optional/Later: userFlagsContainer]
                - linkContainer
                - headingContainer
            - [Optional: linkSpaceContainer]
            - [Optional: imagesContainer]
            - [Optional: branchesContainer]

- Only Horizontal layout supports rotated subcontainers so far
  (Vertical layout does not require rotated elements for foreseeable use cases)

Ideas 
-----

* Introduce *reference position*, which can be used in other containers
  to position these containers. The reference position should be center
  of HeadingContainer

* Introduce *manual positioning* and *autoPositioning*. Manual
  positioning is used for all floating containers, incl. mapCenters and
  MainBranches.

  OTOH: If a container is in a floating layout or if it has no
  parentItem, it uses manual positioning.

* New layout type to handle floats in InnerContainer. Inner Container
  has one of these two layouts:
  - Horizontal (as already in use for non-floating children)

  - *BoundingFloats* when branchesContainer has Floating layout
    
    First child is *targetContainer*, which will be ornamentsContainer,
    next children are (optionally) imagesContainer and branchesContainer.
    (LinkSpaceContainer is not used with BoundingFloats!)

    By using this order, the layout of InnerContainer can be switched
    between Horizontal and BoundingFloats.

    For BoundingFloats:

    bC and iC are processed first to calc bbox of all children relative to OC
        - depending on their rel positions
        - ????
    oC is positioned within BoundingFloats, so that upperleft of bbox of
    children is again (local) origin.
    
* Add/remove containers as needed. Examples
  - Flags, frames, etc. only if set
  - Additional info likes statistics (no. of children, ...)

* Dedicated containers for links
  - Allows more complex links, e.g. curved "bottomlines" under headings,
    maybe curved headings later
  - Proposition to have names on links, maybe rotated to save screen space


Bugs
----

* Real positions
    - Mainbranches and branches need to keep position when detaching
    - Branches keep position when layout changes to float
    - Branches and parent branches (!) keep position, when switching to
      free float
    - keep position when relinking and relinking to rootItem (becoming
      MC)

  Ideas:
    - Reintroduce relative coordinates for mainbranches and floating
      branches

    - Relative coordinates are "real": from center of parent Ornamented
      Container to center of orn. container

    - Mapcenters use real scene positions

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
