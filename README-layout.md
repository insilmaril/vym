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

I have decided to maintain both structures  indepently, which allows
to use the QAbstractItem for the TreeEditor and general maintenance of
the "logical" tree and at the same time use the parent-child relations
of QGraphicsItems for the more complex layouts, like rotated elements
and also simplified positioning algorithms, when different layout
schemes are used.

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

- Container can either hold a MapObj or a set of other Containers
- A container with a HeadingObj is currently parent item to the HO and also all
  QGraphicsTextItems within the HO

- Containers are no longer part of a BranchObj, but become their own
  structures. In the end they will probably replace the current MapObj and
  inherited classes.

  So far there are these container types:

    - BaseContainer
    - BranchContainer
    - HeadingContainer (still pointing to a HeadingObj)

  
Open questions
--------------

- Currently containers are part of BranchObj. Later on they should
  replace the BOs

Ideas 
-----

* Add/remove containers as needed. Examples
  - Flags, frames, etconly if set
  - Additional info likes statistics (no. of children, ...)

* Dedicated containers for links
  - Allows more complex links, e.g. curved "bottomlines" under headings,
    maybe curved headings later
  - Proposition to have names on links, maybe rotated to save screen space


Next steps
----------

* Moving containers around
    - Todo
        - Position LineEdit to container instead of MapObj in
          MapEditor::editHeading
        - Fix z value of select box for selected containers
        - absolute position for MapCenters for COntainers
        - relative position for Mainbranches in COntainers
          ("FramedFloat")
        - Relative position for Images: "FreeFloat" or "FramedFloat"

    - Position containers temporarily while moving: When an object can be
      linked temporarily, it needs a relative position to the tmp parent.

      Several selected branches can be moved (at least later), by temporary
      adding them to the moving tmpParentContainer

      Target destinations then  should offer
      - a link point to draw the start of the tmp link
      - an abs Pos to the tmpParentContainer
      - information where the moved container should be moved, e.g. to the
        left or the right of the link point ("Orientation")
      - maybe also information about rotation of tmp child

      The tmpParentContainer needs to be positioned based on link point and
      orientation from above, also considering modifier keys to link
      above/below. The bounding rect information and also parent needs to
      remain unchanged, to avoid flickering in the tree, but the (relative)
      position itself needs to be adjusted => a new layout hint is required.

    - Animate the back moving. Don't do this in VymModel::startAnimation,
      but in MapEditor

...

* Cleanup MapObj related stuff, which has been replaced by Containers
    - MapEditor
        - Mouse events
            - MousePress
            - MouseMove
            - moveObject
                - [TODO] Update Selection boxes for multiple selected items
            - MouseRelease
                - [DONE] Relink branches above/below/to dst
                - [TODO] Relink images
                - [TODO] Animation to snapback to org pos
        - updateSelection
        ...
    - MapItem
        - Probably pretty much useless once Containers are fully
          implemented. Especially the getEditPosition and
          getSelectionPath should move to containers
    - MapObj
        - need at all in the end?
        - getSelectionPath
