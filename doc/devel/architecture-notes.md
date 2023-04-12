Data structures
================

There are two views on the data:

 * Logical view:

   Defines the logical "tree" structure of the objects in a map. This is
   used for organization (load/save, inserting and deleting branches,
   traversing data for various modifications etc.

   All related changes are organized in the central VymModel class.
   VymModel itself is derived from TreeModel, which focuses on the
   low-level modifications of data, especially
   inserting/deleting/relinking of branches.

 * Graphical view:

   Defines how visible objects are related, e.g. a row of flags, which
   is part of a branch, contains several flags, so the flags are a part
   of the row. This is done via nested "Container" objects.

   Containers provide various layouts and graphical options, like
   defining alingment of contents.

