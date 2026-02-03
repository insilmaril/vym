Release notes VYM - View Your Mind
==================================


The lists below shows main changes in the current version of vym on
develop branch and the last official release 2.9.27.

vym has been rewritten in large parts:
 * New layout engine
   - Supports rotation and scaling of elements
   - Supports frames around subtrees
 * Introducing MapDesign
   - The design of a map can be saved within the map, e.g. default maps
   - MapDesign defines how elements look depending on depth, e.g. which
     frames are used and also the color palettes.
 * New scripting engine
   - Internally the engine was migrated from QScriptEngine to QJSEngine
   - Scripts can be nested now to improve undo/redo handling
   - Various commands have been renamed
   - Abstraction of vym, VymModel, and elements like Branch, Image etc.
     has been implemented in classes available in scripting
     (See the related wrapper classes in C++)

Feel free to report any bugs or feature requests on
[https://github.com/insilmaril/vym/issues](https://github.com/insilmaril/vym/issues)

Thanks for using vym!

Uwe Drechsel - January 2026


## Version 2.9.601
### Bugfixes
 * Bugfix: Enable GoTo target when nothing is selected   (#175)
 * Bugfix: Packaging for MACOSX

### Features
 * Feature: Toggling frames of multiple selected branches (#177)
 * Feature: Open and edit Urls in TextEditors (#176)
 * Feature: Add and follow hyperlinks in TextEditors (#176)
 * Feature: Don't adapt view to show many selected items (#178)
 * Feature: On Macs open history window with Shift-Cmd-H

## Version 2.9.599
### Bugfixes
    Bugfix: Center on selected item not subtree when selecting slide (#153)
    Bugfix: Scroll to selected task in TaskEditor (#173)
    Fixed focus handling with satellite windows (#161, #165)

### Features
    Feature: Switch focus between Find-LineEdit and Find-Results with tab (#165)
    AppStream / Metainfo Improvements (#169)
    cmake: install resources under share/vym and set VYMBASEDIR (#167)

Version: 2.9.598
    Feature: Shortcuts to rotate subtree (Ctrl-R and Ctrl-Shift-R)
    Feature: Support Jira Cloud (#160)
    Feature: Remember last searches in FindResultWidget
    Feature: "Delete" is now equivalent to "Cut"
    Feature: Escape-key cancels editing heading of a branch (#16)
    Bugfix: Background style in TextEditors (#168)
    Bugfix: Undo colorSubtree

    Change: Use CTRL-S for "Restore session" until map was changed
        Then CTRL-S will become "Save map". This frees up the shortcut to rotate
        subtrees with CTRL-R

Version: 2.9.594
    Bugfix: Forced bright theme when system uses dark theme
    Feature: Switch focus between editors using Tab-key (#162)

Version: 2.9.593
    Bugfix: Improved handling of default colors (#80)
    Bugfix: Switch back to MapEditor using Esc-Key (#161)
    Bugfix: Noteeditor has correct window name
    Bugfix: Consider penWidth of frame
    Bugfix: No bottomline for both inner/outer frame
    Bugfix: Frametype Pipe had wrong dimensions

    Feature: Y-Key to yank (copy) in Vim style
    Feature: Draw border around editor which has keyboard focus
    Feature: Exit vym from script
    Feature: Repeat last action for multiple actions

Version 2.9.592
    Feature: Easy following of references: xLinks, Urls and vymLinks
        If a branch has exactly one reference, Key-F will just "follow" this
        reference. Popup menu is only used if multiple references require a
        decision.

Version: 2.9.591
    Bugfix: Better selection color handling TaskEditor
        Colors also no longer depend if selection was clicked in TE or in ME
    Bugfix: Allow macros to work on all selected branches
    Bugfix: Craah when closing map while loading
    Feature: VIM-like shortcuts 0 and $ to select first/last sibling

Version 2.9.590
    Bugfix: Avoid crash when zipAgent is no longer available and zipFinished called
    Bugfix: Always enable fileExitVym action
    Bugfix: HTML export correctly exports flags now
    Feature: Use Key U for undo like in vim
    Update Appstream Data (#158)

Version 2.9.588
    Feature: Improving HTML export with flags and dark theme css file

Version 2.9.587
    Bugfix: Undo/redo for setLinkStyle()
    Bugfix: Remove upLink when detaching mainBranch
    Change: Linkstyle now refers to current branch depth

Version 2.9.586
    Feature: New scripting commands to iterate over branches
    Bugfix: Missing whitespaces in "Goto linked map menu"
    Bugfix: Bigger circle for positioning when relinking to MapCenter

Version 2.9.585
    Bugfix: Intermittent crash when dropping tasks in TaskEditor

Version 2.9.584
    Fixed permissions for new directories (#151)

Version 2.9.583
    Bugfix: Store vym temporary files in user directory (#151)
    Bugfix: Remove warnings by using new svg for "lifebelt" flag
    Bugfix:  #146 signed bundle (#152)
    Bugfix: Toggle subtree frames with function keys
    Feature: Use shared renderer for svg flags (see #151)
Version 2.9.582
    Bugfix: Building on Linux (#149)

Version 2.9.581
    Bugfix: Menu entry to open visible Urls in subtree
    Bugfix: Removed warnings related to old Q\_OS\_MACX macro (#148)

Version 2.9.580
    Feature: On Macs Urls can be opened in private mode in Firefox
    Bugfix: Undo/Redo of changing branches and images layouts
    Bugfix: Fixed script to download image after drop event
    Feature: Allow drag and drop of images without downloads
    Feature: Reset selection size with Ctrl-0 (or Cmd-0)

Version 2.9.578
    Bugfix: Default settings TextEditors (see also #147)


Version 2.9.577
    Feature: Improve listing of keyboard shortcuts
    Fixed history tests. 308 tests available

    Undo/Redo for scaling images
    
Version 2.9.570

Version 2.6.569
    Undo/redo for heading column width and autodesign option
    Improved alignment of info in ExtraInfoDialog

Version 2.9.567
    Feature: Initial support to import IThoughts maps
Version 2.9.564
    Bugfix: No longer ignore "Accept" in downloads dialog (#136)
Version 2.9.563
    Don't use STREQUAL on Max (#135)
    Minor improvements CMakeLists.txt
Version 2.9.562
    Fixed when moving branches up

Version 2.9.561
Version 2.9.560
    Bugfix: Crashes when running scripts
    Add io.github.insilmaril.vym.appdata.xml (#123)
    Remove shortkey conflict for Key\Plus
    Bugfix: Don't add command from last saveState script, if no script is used

    Feature: Show keyboard shortcuts in context menus
    Feature: On Macs use backspace as shortcut instead of delete

    Version 2.9.558

    Feature: New dialog for logfile settings

    Stop view animations when wheel is used for scrolling

    Feature: Toggle temporary hide mode
    
    Feature: Temporary hide parts of map
        "Clouded" branches are not exported and can be temporary hidden.
        This commit also fixes, that clouded parts are still saved, even while
        invisible.
    Bugfix: Confluence export with Urls containing ampersands

    Bugfix: Deleting children in a new map without path, might cause hang

    Bugfix: Crash with dangling xlinks

    Bugfix: darkTheme handling

    Version 2.9.557

    Feature: New icons in MainWindow for more modern look
        - based on KDE breeze
        - prepared for theming (bright, dark, classic)

    Feature: #35 Insert images in NoteEditor


    Feature: New icons for NoteEditor and HeadingEditor

    Feature: New icon to edit fill color in TextEditors

    Fixed crash related to QJSEngine
        If a scriptEngine was destroyed in MainWindow, the engine also deleted
        the vymWrapper due to wrong ownership.
        
        Subsequent scriptEngines got initialized with dangling vymWrapper
        pointer


    Version 2.9.556

    Changed logging when requesting release notes
        - Write vymCodeQuality in MainWindow::serverUrl()
        - read vymCodeQuality on server in handler.php
        - no longer write and read "config" parameter

    Version 2.9.555
    Bugfix: Adjust viewport size when moving items

    Version 2.9.554

    Feature: Get Confluence page last edit details
        Add attributes for
        - Author
        - Timestamp

    Update about dialog to reflect current reality. (#130)

    Feature: Get labels from Confluence and modify them
        - Recursively get page tree of a given page
        - Update attributes with metadata from Confluence
        - Added script command to delete label in Confluence
        - Added demo scripts to find and delete labels from pages

    Fixed iterating branches in scripts. Improved Confluence page details handling

    Bugfix: Updating heading from Confluence URL won't save color

    Get Confluence labels as part of pageInfo
        
        - Updates heading of branch with page title
        - Adds page labes as attributes to branch

    Version 2.9.553

    Bugfix: Crash when deleting XLinks
    Bugfix: Set and unset Url flag correctly


    Version 2.9.552

commit e8268cbbfe40425802b2f1f5677115ce105d408e
Author: Uwe Drechsel <vym@insilmaril.de>
Date:   Thu Nov 7 15:19:09 2024 +0100

    Minor logging improvement

commit 1d29dd23b1122d049e86d17db2c8bf9f5a6a01e5
Author: Uwe Drechsel <vym@insilmaril.de>
Date:   Wed Nov 6 09:56:28 2024 +0100

Version 2.9.551
    Feature: Alignment of imagesContainer relative to branchesContainer
    Bugfix: #95 LibreOffice Impress export improved
    Bugfix: Redo adding xlink
    Bugfix: Toggling flags in groups (incl. undo/redo)
    Ported tests for flags
    Partially update mainbranches and their children during load
Version 2.9.550
    Ported tests for legacy maps
    Bugfix: undo/redo of setHideLinkUnselected() and setHideExport()
    Bugfix: undo/redo loadImage()
    Bugfix: undo/redo toggleTarget()
    Bugfix: undo/redo for rotations and scalings
    Bugfix: TaskJuggler export including XSL transformation
    Bugfix: Save position when moving images
    Feature: undo/redo modifying attributes
version 2.9.549
    Bugfix: add branch before (including undo/redo in one step)
Version 2.9.548
    Feature: Adding a branch and editing heading only has one undo step now
    Bugfix: #126 write flags in XML export (impproved)
    Bugfix: #126 Write userflags in XML export
    Feature: Only one undo/redo step for adding MapCenters
    Bugfix: deleting XLinkItem left XLink dangling around
Version 2.9.547
    Use multiple and local scriptEngines
Version 2.9.546
    Feature: Use special Url flag for Jira tickets
        If the attribute "Jira.issueUrl" is set, the system-jira flag will be
        used instead of the system-url flag.
        
        TreeItem class has new methods to set and get the urlType, currently
        GeneralUrl and JiraUrl.
        
        Queries are not really supported yet.
    Change: Disabled shortcut to switch to RichText Ctrl-R in TextEditor
        Conflicts with shortcut to restore session

    Feature: Improved editing of cells in the TaskEditor. For Issue #12. (#121)
    Allow nested branch iterators in scripting
    Bugfix: Crash when using bright theme #120
    Added icon to Bleyddyns patch to clear recent files #95
    Clear recent menu item (#119)
        * Added a menu item to clear the recent map menu.
        * Changed menu item name to just 'Clear'. For Issue #95.
Version 2.9-545
    Feature: New frame type "Pipe"
    Feature: Changing frame colors updates map instantly
    Feature: While loading update MapEditor #118
    Feature: Use system services to open local and remote Urls
        By default system specific apps will be used to open pdfs, webpages,
        spreadsheets, ...
    Finalized fix for #98 (hopefully)
        - Set lastMapDir also for Main::fileSaveAs()
    Update lastMapDir in more places. Fix for #98. (#117)
Version 2.9.544
    Feature: Improved selection of items in map with keyboard
        Introduced selection modes based layout and geometry, e.g. navigating in
        grids and orgcharts works now as expected. Also jumping from a branch to
        nearest image.
    Feature: Select nearest branch below current one
    Feature: Set Jira ticket ID in heading
Version 2.9.543
    Fixed XLink related scripting
    Fixed restoring window geometry on Windows
Version 2.9.542
    Fixed test to add branch above/below
    Fixed adding above/below with undo/redo
    Unified shortcuts for adding branches
        - [Key_A] with modifiers to add
            - as child of selection
            - insert before selection [Shift + Ctrl]
            - above selection [Shift]
            - below selection [Ctrl]
Version 2.9.538
    Fixed segfault when exiting vym after running selftests
    Feature: Update Russian translations vym.ru.ts (#115)

    Change: Reworked scripting commands
        Moved from VymModelWrapper to BranchWrapper
        - branchCount()
        - clearFlags()
        - colorBranch()
        - colorSubtree()
        - selectFirstBranch()
        - selectLastBranch()
        - selectParent()
        
        Adapted tests
Version 5.9.537
    Feature: Macro to toggle task considers dark theme.
    Use new mode file sync (-FS) when zipping directories on Linux and Mac.
        Seems also to work similar on Windows 11 using tar.

Version 2.9.536
    Reworked saveState and handling of notes and headings
        - Continues to work on new saveState function, which no longer uses
          undo/redo selection, but uses uses script commands on specific
          branches/images, which are found using Uuid.
        - Removed parseVymText to to set notes and headings
        - Introduced a number of new commands for branches and images
        - Introduced ImageWrapper to allow applying commands on images
        - Introduced command to check availability of dark theme


Version 2.9.535
    Bugfix: Heading color is lost when note is available (#113)

Version 2.9.534
    Change: XLinks now use UUID instead of selectionID (#112)
        This allows processing XLinks also in XSL transformations. See

Version 2.9.533

    Bugfix: Ampersands in notes exported to libreoffice impress
    Fixed unzip on Windows

Version 2.9.28
    Feature: link app icon as a mimetype icon for the hicolor default theme (#109)
        On Linux/Unix systems cmake already installs the vym.png application
        icon (what is referenced via the .desktop file) and a mime type definition
        for `application/x-vym`. What is missing is the icon to use on `.vym`
        files which are associated with this mime type. Instead of installing
        the icon a second time, a relativ symlink is created referencing the app
        icon.
    Fix: do not install manpage in doc dir (#107)
    Bugfix: Open french documention if required
    Fixed: Don't autosave while still saving
    Spelling fix: remove duplicate word (#108)
Version 2.9.532
    Run zip process in foreground when exporting to libreoffice impress.
    zip running as background process on Linux and Mac.
        Windows not ported yet.
    Spelling fixes (#105)
    Desktop file improvements (#106)
    Feature: Only write images once to zipDir to save time and disk space

