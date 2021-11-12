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

  
Open questions
--------------

- Currently containers are part of BranchObj. Later on they should
  replace the BOs

