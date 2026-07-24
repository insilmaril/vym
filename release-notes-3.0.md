Release notes VYM - View Your Mind
==================================


The lists below shows main changes between the current 3.0 version of vym and
the previous official release 2.9.27.

vym has been rewritten in large parts:

 * New layout engine
   - Supports rotation and scaling of elements
   - Supports frames around subtrees
   - Transparency for frame colors

 * Introducing MapDesign
   - Mapdesign defines how a map looks visually, e.g. colors, frames,
     links
   - The design can be saved within the map, e.g. with the default map,
     which is loaded initially
   - MapDesign defines how elements look depending on depth, e.g.
     MapCenters and first level MainBranches may have frames, other
     branches not.

 * More personalization options
   - Mapdesign above allows to save personal preferences for designs in
     default maps or easily share them
   - Theming has improved
     - Dark mode support
     - New icons and additional icons for dark mode
     - Even selection box can be styled (so far only be modifying
       mapdesign in xml file, e.g. using vivym script)

 * Improved text handling in NoteEditor and HeadingEditor
     - RichText in headings of branches
        - Copy & paste information including bulletpoints and even
          images into the heading of a branch
     - Improved color handling
        - New shortcuts to color text (Ctrl-T) and "mark" background (Ctrl-M)
        - Color buttons in toolbar no longer update to current cursor color,
          but remember previously selected colors
     - [#176](https://github.com/insilmaril/vym/issues/176)  Add and follow hyperlinks in TextEditors
     - [#168](https://github.com/insilmaril/vym/issues/168)  Background style in TextEditors

 * Keyboard shortcuts
   - Unified shortcuts for satellite editors
     - One key to open a specific editor, e.g. N for NoteEditor, B for Properties
     - One key to close each editor: Ctrl-D
   - More shortcuts inspired by vim editor without Ctrl/Cmd key:
     - D to delete
     - Y to yank/copy
     - P to paste
     - 0/$ to go to first/last branch in current subtree
     - U Undo
     - / Find
     - Repeat last action with .
   - Move a branch up/down "diagonally" by making it a sibling or a
     child
   - Zoom view easily with + and - (without Ctrl)  
   - Resize items easily with Ctlr-+/-
   - Center view on item and reset zoom factor with ,
   - Center view and zoom in to item with Shift-,
   - Move view to make item visible with #
   - Heading editor and note editor: color text (Ctrl-T) and "mark" background (Ctrl-M)


 * Speedup and optimizations
   - The layout is calculated only once when data related to graphics
     change (internally: less calls to reposition() function)
   - When maps are saved, vym still zips the XML-data, but the
     compression is done as background process, while vym already is
     responsive to user input again.

 * New scripting engine
   - Internally the engine was migrated from (no longer supported)
     QScriptEngine to QJSEngine
   - Scripts can be nested now to improve undo/redo handling and
     automated tests
   - Various commands have been renamed
   - Abstraction of vym, VymModel, and elements like Branch, Image etc.
     has been implemented in classes available in scripting (See the
     related wrapper classes in C++ source code for details)

Feel free to report any bugs or feature requests on
[https://github.com/insilmaril/vym/issues](https://github.com/insilmaril/vym/issues)

Thanks for using vym!

Uwe Drechsel - July 2026

## Unreleased
### Bugfixes
 * [#150](https://github.com/insilmaril/vym/issues/150) No more redundant overwrite confirmation on macOS when saving a map, saving an image or exporting; the manual confirmation is kept on Windows and Linux
 * [#174](https://github.com/insilmaril/vym/issues/174) Tasks now visible in a new map created from a command-line filename (TaskEditor map filter was left stale)
 * [#218](https://github.com/insilmaril/vym/issues/218) Scroll/unscroll of a branch while it is animated no longer breaks its positioning
 * Bugfix: MapEditor animation settings not only considered when map is created
 * Fixed floating branches (e.g. mainbranches) jumping to a wrong position when dragged and released without relinking; they now stay where they are dropped
 * Fixed the first (or a single) dragged branch briefly animating to a wrong position inside the temporary move container
 * Fixed downlink to children starting at the branch center (as for a MapCenter) while a mainbranch is being dragged; the original downlink position is now kept
 * Fixed orientation of floating children flipping and flickering while their parent branch is being dragged; the orientation is now kept stable during the drag

## Version 3.0.1
### Bugfixes
 * Fixed wrong x/y positions of branches while moving multiple selected branches with the mouse; they are now animated into their stacked positions (if animations are enabled)
 * [#217](https://github.com/insilmaril/vym/issues/217) Moving selected branches to a target can be repeated using "."-key
 * [#216](https://github.com/insilmaril/vym/issues/216) Find results don't refresh when switching to another map

## Version 2.9.617
### Bugfixes
 * [#210](https://github.com/insilmaril/vym/issues/210) BranchPropertyEditor not updated when changing maps

## Version 2.9.616
### Features
 * Script stats.vys to analyze sizes of subtrees

### Bugfixes
 * [#207](https://github.com/insilmaril/vym/issues/207) Cmd-D not working to close Script output window
 * [#206](https://github.com/insilmaril/vym/issues/206) Two dialogs when creating new vymLink
 * Fix building with Qt 6.6

## Version 2.9.614
### Bugfixes
 * [#189](https://github.com/insilmaril/vym/issues/189) Updated demo maps

### Changes
 * Only ask once to allow vym to download release notes and check for
   updates

## Version 2.9.613
### Features
 * Feature: Notes in Html export are collapsible

### Bugfixes
 * [#204](https://github.com/insilmaril/vym/issues/204) Crash after image/branch manipulation

## Version 2.9.612
### Features
 * [#197](https://github.com/insilmaril/vym/issues/197) Improved function key handling
   - Add shortcuts to color only branch, not whole subtree
 * [#194](https://github.com/insilmaril/vym/issues/194) Group keyboard shortcuts

### Bugfixes
 * [#196](https://github.com/insilmaril/vym/issues/196) Xlink control points not accessible when "behind" heading
 * Load translations on Mac

## Version 2.9.610
### Bugfixes
 * [#195](https://github.com/insilmaril/vym/issues/195) Save background color when exporting RichText in TextEditor

## Version 2.9.609
### Features
 * Improved color handling in text editors
    - New shortcuts to color text (Ctrl-T) and "mark" background (Ctrl-M)
    - Color buttons in toolbar no longer update to current cursor color,
      but remember previously selected colors

### Bugfixes
 * Remove double overwrite confirmation on Mac for exporting notes

## Version 2.9.608
### Bugfixes
 * When importing maps, don't read mapdesign.
 * [#191](https://github.com/insilmaril/vym/issues/191) AddMapReplace crashes for MapCenter 

### Changes
 * When clicking on the map, find branches before XLinks

## Version 2.9.607
### Bugfixes
 * Branches without frames
 * [#185](https://github.com/insilmaril/vym/issues/185)  After loading a zoomed map is not centered correctly

## Version 2.9.606
### Bugfixes
 * [#181](https://github.com/insilmaril/vym/issues/181)  Relative positioning of images or branches

## Version 2.9.605
### Bugfixes
 * autoDesign frame settings when loading/saving maps
 * [#188](https://github.com/insilmaril/vym/issues/188)  save selection
    - Added script command isBusy()
    - Added script command setSaveAsBackgroundProcess()

    Changed handling of zipping in background. Maps were renamed before zip
    was finished. For testing added option to wait for finishing zip as
    foreground process, otherwise tests would have failed.

    Also re-added action to file menu to save selection.

 * [#187](https://github.com/insilmaril/vym/issues/187)  Make sure zip processes are finished when leaving vym

    Under certain circumstances vym had still background processes when
    accepting e.g. a close event from window manager.

    Now MainWindow requests closing a map directly from the vym map. The map
    itself triggers it's removal from MainWindow once the zipProcess is done.

 * [#187](https://github.com/insilmaril/vym/issues/187)  Missing image in xml of map modifies branches


## Version 2.9.604
### Bugfixes
* [#180](https://github.com/insilmaril/vym/issues/180)  in TaskEditor adds heading of parent branch to selected branch
* [#179](https://github.com/insilmaril/vym/issues/179)  Changes to frames via script are not saved 

   e.g. when using macros bound to function keys to change frames.

   Now autoDesign is disabled when script functions change frames.

* [#186](https://github.com/insilmaril/vym/issues/186)  Exporting note might overwrite export of previous branch
* [#182](https://github.com/insilmaril/vym/issues/182)  Zoom and rotation after loading map
* [#183](https://github.com/insilmaril/vym/issues/183)  Rotation of subtree not save when changed with shortcut
  - Disable autodesign option


## Version 2.9.603
### Features
 * Set heading width also for RichText headings

### Bugfixes
 * QThreadStorage: entry 1 destroyed before end of thread when quitting via shortcut cut

## Version 2.9.601
### Features
 * [#177](https://github.com/insilmaril/vym/issues/177)  Toggling frames of multiple selected branches
 * [#176](https://github.com/insilmaril/vym/issues/176)  Open and edit Urls in TextEditors
 * [#176](https://github.com/insilmaril/vym/issues/176)  Add and follow hyperlinks in TextEditors
 * [#178](https://github.com/insilmaril/vym/issues/178)  Don't adapt view to show many selected items
 * On Macs open history window with Shift-Cmd-H

### Bugfixes
 * [#175](https://github.com/insilmaril/vym/issues/175)  Enable GoTo target when nothing is selected
 * Packaging for MACOSX


## Version 2.9.599
### Features
 * [#165](https://github.com/insilmaril/vym/issues/165)  Switch focus between Find-LineEdit and Find-Results with tab
 * [#169](https://github.com/insilmaril/vym/issues/169)  AppStream / Metainfo Improvements
 * [#167](https://github.com/insilmaril/vym/issues/167)  cmake: install resources under share/vym and set VYMBASEDIR

### Bugfixes
 * [#153](https://github.com/insilmaril/vym/issues/153)  Center on selected item not subtree when selecting slide
 * [#173](https://github.com/insilmaril/vym/issues/173)  Scroll to selected task in TaskEditor
 * [#161](https://github.com/insilmaril/vym/issues/161) , [#165](https://github.com/insilmaril/vym/issues/165)  Focus handling with satellite windows 

## Version 2.9.598
### Changes
 * Use CTRL-S for "Restore session" until map was changed

    Then CTRL-S will become "Save map". This frees up the shortcut to rotate
    subtrees with CTRL-R

### Features
 * Shortcuts to rotate subtree (Ctrl-R and Ctrl-Shift-R)
 * [#160](https://github.com/insilmaril/vym/issues/160)  Support Jira Cloud
 * Remember last searches in FindResultWidget
 * "Delete" is now equivalent to "Cut"
 * [#16](https://github.com/insilmaril/vym/issues/16)  Escape-key cancels editing heading of a branch

### Bugfixes
 * [#168](https://github.com/insilmaril/vym/issues/168)  Background style in TextEditors
 * Undo colorSubtree

Version 2.9.594
### Features
 * [#162](https://github.com/insilmaril/vym/issues/162)  Switch focus between editors using Tab-key

### Bugfixes
 * Bugfix: Forced bright theme when system uses dark theme

## Version 2.9.593
### Features
 * Y-Key to yank (copy) in Vim style
 * Draw border around editor which has keyboard focus
 * Exit vym from script
 * Repeat last action for multiple actions

### Bugfixes
 * [#80](https://github.com/insilmaril/vym/issues/80)  Improved handling of default colors
 * [#161](https://github.com/insilmaril/vym/issues/161)  Switch back to MapEditor using Esc-Key
 * Noteeditor has correct window name
 * Consider penWidth of frame
 * No bottomline for both inner/outer frame
 * Frametype Pipe had wrong dimensions

## Version 2.9.592
### Features
 * Easy following of references: xLinks, Urls and vymLinks

    If a branch has exactly one reference, Key-F will just "follow" this
    reference. Popup menu is only used if multiple references require a
    decision.

## Version 2.9.591
### Features
 * VIM-like shortcuts 0 and $ to select first/last sibling

### Bugfixes
 * Better selection color handling TaskEditor

      Colors also no longer depend if selection was clicked in TE or in ME

 * Allow macros to work on all selected branches
 * Craah when closing map while loading

## Version 2.9.590
### Features
 * Use Key U for undo like in vim

### Bugfixes
 * Avoid crash when zipAgent is no longer available and zipFinished called
 * Always enable fileExitVym action
 * HTML export correctly exports flags now
 * [#158](https://github.com/insilmaril/vym/issues/158)  Update Appstream Data


## Version 2.9.588
### Features
 * Improving HTML export with flags and dark theme css file

## Version 2.9.587
### Changes
 * Linkstyle now refers to current branch depth

### Bugfixes
 * Undo/redo for setLinkStyle()
 * Remove upLink when detaching mainBranch

## Version 2.9.586
### Features
 * New scripting commands to iterate over branches

### Bugfixes
 * Missing whitespaces in "Goto linked map menu"
 * Bigger circle for positioning when relinking to MapCenter

## Version 2.9.585
### Bugfixes
 * Intermittent crash when dropping tasks in TaskEditor

## Version 2.9.584
### Bugfixes
 * [#151](https://github.com/insilmaril/vym/issues/151)  Fixed permissions for new directories

## Version 2.9.583
### Features
 * [#151](https://github.com/insilmaril/vym/issues/151)  Use shared renderer for svg flags

### Bugfixes
 * [#151](https://github.com/insilmaril/vym/issues/151)  Store vym temporary files in user directory
 * Remove warnings by using new svg for "lifebelt" flag
 * [#152](https://github.com/insilmaril/vym/issues/152)  signed bundle
 * Toggle subtree frames with function keys


## Version 2.9.582
### Bugfixes
 * [#149](https://github.com/insilmaril/vym/issues/149)  Building on Linux

## Version 2.9.581
### Bugfixes
 * Menu entry to open visible Urls in subtree
 * [#148](https://github.com/insilmaril/vym/issues/148)  Removed warnings related to old Q\_OS\_MACX macro

## Version 2.9.580
### Features
 * On Macs Urls can be opened in private mode in Firefox
 * Allow drag and drop of images without downloads
 * Reset selection size with Ctrl-0 (or Cmd-0)

### Bugfixes
 * Undo/Redo of changing branches and images layouts
 * Fixed script to download image after drop event

## Version 2.9.578
### Bugfixes
 * [#147](https://github.com/insilmaril/vym/issues/147)  Default settings TextEditors

## Version 2.9.577
### Features
 * Improve listing of keyboard shortcuts

### Bugfixes
 * Fixed history tests. 308 tests available
 * Undo/Redo for scaling images
    
## Version 2.6.569
### Bugfixes
 * Undo/redo for heading column width and autodesign option
 * Improved alignment of info in ExtraInfoDialog

## Version 2.9.567
### Features
 * Initial support to import IThoughts maps

## Version 2.9.564
### Bugfixes
 * [#136](https://github.com/insilmaril/vym/issues/136)  No longer ignore "Accept" in downloads dialog ([#136](https://github.com/insilmaril/vym/issues/136) )

## Version 2.9.563
### Bugfixes
 * [#135](https://github.com/insilmaril/vym/issues/135)  Don't use STREQUAL on Max
 * Minor improvements CMakeLists.txt

## Version 2.9.562
### Bugfixes
 * Fixed when moving branches up

## Version 2.9.560
### Features
 * Show keyboard shortcuts in context menus
 * On Macs use backspace as shortcut instead of delete

### Bugfixes
 * Crashes when running scripts
 * [#123](https://github.com/insilmaril/vym/issues/123)  Add io.github.insilmaril.vym.appdata.xml
 * Remove shortkey conflict for Key\Plus
 * Bugfix: Don't add command from last saveState script, if no script is used


## Version 2.9.558
### Features
 * New dialog for logfile settings
 * Toggle temporary hide mode
 * Temporary hide parts of map

   "Clouded" branches are not exported and can be temporary hidden.
   This commit also fixes, that clouded parts are still saved, even while
   invisible.

### Bugfixes
 * Stop view animations when wheel is used for scrolling
 * Confluence export with Urls containing ampersands
 * Deleting children in a new map without path, might cause hang
 * Crash with dangling xlinks
 * darkTheme handling

## Version 2.9.557
### Features
 * New icons in MainWindow for more modern look
   - based on KDE breeze
   - prepared for theming (bright, dark, classic)
 * [#35](https://github.com/insilmaril/vym/issues/35)  Insert images in NoteEditor
 * New icons for NoteEditor and HeadingEditor
 * New icon to edit fill color in TextEditors

### Bugfixes
 * Fixed crash related to QJSEngine
   If a scriptEngine was destroyed in MainWindow, the engine also deleted
   the vymWrapper due to wrong ownership.
        
   Subsequent scriptEngines got initialized with dangling vymWrapper
   pointer

## Version 2.9.555
### Bugfixes
 * Adjust viewport size when moving items

## Version 2.9.554
### Features
 * Get Confluence page last edit details

   Add attributes for
    - Author
    - Timestamp
 * [#130](https://github.com/insilmaril/vym/issues/130)  Update about dialog to reflect current reality.
 * Get labels from Confluence and modify them
   - Recursively get page tree of a given page
   - Update attributes with metadata from Confluence
   - Added script command to delete label in Confluence
   - Added demo scripts to find and delete labels from pages

### Bugfixes
 * Fixed iterating branches in scripts. Improved Confluence page details handling
 * Updating heading from Confluence URL won't save color
 * Get Confluence labels as part of pageInfo
   - Updates heading of branch with page title
   - Adds page labes as attributes to branch

## Version 2.9.553

### Bugfixes
 * Crash when deleting XLinks
 * Set and unset Url flag correctly

## Version 2.9.551
### Features
 * Alignment of imagesContainer relative to branchesContainer

### Bugfixes
 * [#95](https://github.com/insilmaril/vym/issues/95)  LibreOffice Impress export improved
 * Redo adding xlink
 * Toggling flags in groups (incl. undo/redo)
 * Ported tests for flags
 * Partially update mainbranches and their children during load

## Version 2.9.550
### Features
 * undo/redo modifying attributes
 * Ported tests for legacy maps

### Bugfixes
 * undo/redo of setHideLinkUnselected() and setHideExport()
 * undo/redo loadImage()
 * undo/redo toggleTarget()
 * undo/redo for rotations and scalings
 * TaskJuggler export including XSL transformation
 * Save position when moving images

## Version 2.9.549
### Bugfixes
 * add branch before (including undo/redo in one step)

## Version 2.9.548
### Features
 * Adding a branch and editing heading only has one undo step now
 * Only one undo/redo step for adding MapCenters

### Bugfixes
 * [#126](https://github.com/insilmaril/vym/issues/126)  write flags and userflags in XML export
 * deleting XLinkItem left XLink dangling around

## Version 2.9.547
 * Use multiple and local scriptEngines

## Version 2.9.546
### Features
 * Use special Url flag for Jira tickets

   If the attribute "Jira.issueUrl" is set, the system-jira flag will be
   used instead of the system-url flag.
        
   TreeItem class has new methods to set and get the urlType, currently
   GeneralUrl and JiraUrl.
        
   Queries are not really supported yet.

 * [#121](https://github.com/insilmaril/vym/issues/121)  Improved editing of cells in the TaskEditor.

### Changes
 * Disabled shortcut to switch to RichText Ctrl-R in TextEditor
   Conflicts with shortcut to restore session

### Bugfixes
 * [#120](https://github.com/insilmaril/vym/issues/120)  Crash when using bright theme 
 * Added icon to Bleyddyns patch to clear recent files ([#95](https://github.com/insilmaril/vym/issues/95) )
 * [#119](https://github.com/insilmaril/vym/issues/119)  Clear recent menu item
   - Added a menu item to clear the recent map menu.
   - Changed menu item name to just 'Clear'. For Issue ([#95](https://github.com/insilmaril/vym/issues/95) )

## Version 2.9.545
### Features
 * New frame type "Pipe"
 * Changing frame colors updates map instantly
 * [#118](https://github.com/insilmaril/vym/issues/118)  While loading update MapEditor
 * Use system services to open local and remote Urls

   By default system specific apps will be used to open pdfs, webpages,
   spreadsheets, ...

### Bugfixes
 * Finalized fix for ([#98](https://github.com/insilmaril/vym/issues/98) )
   - Set lastMapDir also for Main::fileSaveAs()
 * [#117](https://github.com/insilmaril/vym/issues/117)  Update lastMapDir in more places. Fix for [#98](https://github.com/insilmaril/vym/issues/98) .

## Version 2.9.544
### Features
 * Improved selection of items in map with keyboard

   Introduced selection modes based layout and geometry, e.g. navigating in
   grids and orgcharts works now as expected. Also jumping from a branch to
   nearest image.
 * Select nearest branch below current one
 * Set Jira ticket ID in heading

## Version 2.9.543
### Bugfixes
 * Fixed XLink related scripting
 * Fixed restoring window geometry on Windows

## Version 2.9.542
### Bugfixes
 * Fixed test to add branch above/below
 * Fixed adding above/below with undo/redo

### Changes
 * Unified shortcuts for adding branches

   - [Key_A] with modifiers to add
   - as child of selection
   - insert before selection [Shift + Ctrl]
   - above selection [Shift]
   - below selection [Ctrl]

## Version 2.9.538
### Features
 * [#115](https://github.com/insilmaril/vym/issues/115)  Update Russian translations vym.ru.ts

### Bugfixes
 * Fixed segfault when exiting vym after running selftests

### Changes
 * Reworked scripting commands:

   Moved from VymModelWrapper to BranchWrapper
    - branchCount()
    - clearFlags()
    - colorBranch()
    - colorSubtree()
    - selectFirstBranch()
    - selectLastBranch()
    - selectParent()
        
## Version 5.9.537
### Features
  * Macro to toggle task considers dark theme.

    Use new mode file sync (-FS) when zipping directories on Linux and Mac.
        Seems also to work similar on Windows 11 using tar.

## Version 2.9.536
### Changes
 * Reworked saveState and handling of notes and headings
   - Continues to work on new saveState function, which no longer uses
     undo/redo selection, but uses uses script commands on specific
     branches/images, which are found using Uuid.
   - Removed parseVymText to to set notes and headings
   - Introduced a number of new commands for branches and images
   - Introduced ImageWrapper to allow applying commands on images
   - Introduced command to check availability of dark theme


## Version 2.9.535
### Bugfixes
 * [#113](https://github.com/insilmaril/vym/issues/113)  Heading color is lost when note is available

## Version 2.9.534
### Changes
 * [#112](https://github.com/insilmaril/vym/issues/112)  XLinks now use UUID instead of selectionID

   This allows processing XLinks also in XSL transformations. See

## Version 2.9.533
### Bugfixes
 * Ampersands in notes exported to libreoffice impress
 * Fixed unzip on Windows

## Version 2.9.28
### Features
 * [#109](https://github.com/insilmaril/vym/issues/109)  Link app icon as a mimetype icon for the hicolor default theme

 * On Linux/Unix systems cmake already installs the vym.png application
   icon (what is referenced via the .desktop file) and a mime type definition
   for `application/x-vym`. What is missing is the icon to use on `.vym`
   files which are associated with this mime type. Instead of installing
   the icon a second time, a relativ symlink is created referencing the app
   icon.

### Bugfixes
 * [#107](https://github.com/insilmaril/vym/issues/107)  Do not install manpage in doc dir 
 * Open french documention if required
 * Don't autosave while still saving
 * [#108](https://github.com/insilmaril/vym/issues/108)  Spelling fix: remove duplicate word

## Version 2.9.532
### Changes
 * Run zip process in foreground when exporting to libreoffice impress.
 * zip running as background process on Linux and Mac.

   Windows not ported yet.

### Features
 * Only write images once to zipDir to save time and disk space

### Bugfixes
 * [#105](https://github.com/insilmaril/vym/issues/105)  Spelling fixes
 * [#106](https://github.com/insilmaril/vym/issues/106)  Desktop file improvements

