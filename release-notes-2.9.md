Release notes VYM - View Your Mind
==================================


The lists below shows differences between vym 2.8.0 and the latest 2.9.x version.

Feel free to report any bugs or feature requests on
[https://github.com/insilmaril/vym/issues](https://github.com/insilmaril/vym/issues)

Thanks for using vym!

Uwe Drechsel - May 2024

## Version 2.9.26
### Bugfixes
 * Bugfix: Markdown improvements by Markus Seilnacht
 * Bugfix: Allow saving of readonly maps
 * Bugfix: Use vymBaseDir when setting up libreoffice export
 * Bugfix: Resetting task delta prio not limited to visible tasks


## Version 2.9.26
### Features
* Feature: [#87](https://github.com/insilmaril/vym/issues/87) Enable Crtl modifier for macros triggered by function keys.
* Feature: Set last export type to "Update" after successfully "Creating" Confluence page
* Feature: Updated status bar messages when loading/saving maps
* Feature: [#91](https://github.com/insilmaril/vym/issues/91) Update Italian translation
* Feature: Dropped URLs are truncated at start of parameters

### Bugfixes
* Bugfix: Minor typo in German translation
* Bugfix: Minor typo in Confluence settings dialog
* Bugfix: Pasted text URLs in a heading no longer create URL in branch
* Bugfix: [#90](https://github.com/insilmaril/vym/issues/90) Disable BSP indexing to avoid crashes
* Bugfix: [#88](https://github.com/insilmaril/vym/issues/88) Improved ASCII export

### Changes
* Change: Removed or changed shortcuts with ALT

## Version 2.9.22
### Features

* Feature: Support multiple Jira instances with specific authentication methods
* Feature: Floppy disk icon for unsaved maps better visible, if active
* Feature: Upload images also for new Confluence pages
* Feature: New commands to control visuals of selection box
* Feature: added script to set colors of selection box for dark theme:
    - demos/scripts/setSelectionBoxDarkTheme.vys
* Feature: Center on selection and fit to view with Key_Period + Key_Shift
* Feature: Delete vymLink with Ctrl-Shift click
* Feature: Scale pasted images to 300px width
* Feature: Define pen color, width and brush for selection box
    - setSelectionPenColor
    - setSelectionPenWidth
    - setSelectionBrushColor
* Feature: Option to 'never' use dark theme
* Feature: Support Personal Access Tokens for Jira and Confluence
* Feature: Improved animated centering on selection (Shortcut: Key_Period)
* Feature: For multiple selected items show count in status line
* Feature: Scale images on the fly  
    This allows to paste and shrink images (Ctrl + "-"), but when zooming in
    the images are not pixelized, but still have original resolution.
    Storing e.g. screenshots is more efficient this way.

### Bugfixes

* Bugfix: Creating Confluence page without attachments
* Bugfix: Icon and status of view icons
* Bugfix: disabled all icons when no map is available.
* Bugfix: Fixed layout of dialog for Confluence export
* Bugfix: Improved adding new branches at border of current scene
* Bugfix: Urls and VymLinks shown again in statusBar
* Bugfix: Wrong positon of selection box of xlinks control points, resulting in crazy scrolling, when control point is selected.
* Bugfix: [#79](https://github.com/insilmaril/vym/issues/79) quell linking error (#79)
* Bugfix: Set selection background color in TreeEditor
* Bugfix: Also center on selected branch when using HeadingEditor while editing a heading
* Bugfix: Save colors of headings
* Bugfix: Editing long plainText headings might open HeadingEditor
* Bugfix: [#65](https://github.com/insilmaril/vym/issues/65) and #71 Colors in NoteEditor with RichText
* Bugfix: [#76](https://github.com/insilmaril/vym/issues/76) Editing heading of zoomed in view causes panning
* Bugfix: When zooming in/out using mouse wheel don't change rotation
* Bugfix: Background colors in HeadingEditor
* Bugfix: [#40](https://github.com/insilmaril/vym/issues/40) Editing PlainText headings with linebreaks
* Bugfix: [#75](https://github.com/insilmaril/vym/issues/75) TreeEditor and Linebreaks in headings
* Bugfix: [#73](https://github.com/insilmaril/vym/issues/73) Default maps should not have word default in MapCenter
* Bugfix: [#72](https://github.com/insilmaril/vym/issues/72) Improved support to load new default maps
* Bugfix: [#74](https://github.com/insilmaril/vym/issues/74) HTML export uses word wrap for PlainText notes
* Bugfix: Update HeadingEditor for RichText heading, when frame background changes
* Bugfix: Update color and heading of HeadingEditor
* Bugfix: [#70](https://github.com/insilmaril/vym/issues/70) HeadingEditor doesn't use map background when switching on RichText mode
* Bugfix: [#70](https://github.com/insilmaril/vym/issues/70) settings override macroPath, if local option is used "-l"
* Bugfix: [#68](https://github.com/insilmaril/vym/issues/68) HeadingEditor doesn't update after in MapEditor
* Bugfix: Consider zoomFactor after load when scrolling to selection
* Bugfix: Set color and width of legacy xlink

### Changes

* Change: Use Control modifier instead of Shift to only move MapCenter
* Change: Compatibility with 2.9.514: Some elements can be read, even if vym



## Version 2.9.2

### Bugfixes
* Bugfix: #64 Read notes correctly from (very old) maps

## Version 2.9.0
This version provides bugfixes and some new features. The biggest
and most visible changes are dark theme support and an extended color bar
to select colors. 

The platform support has been improved, native Mac version is available
(again) and also binaries for various Linux flavors. See the 
[README.md](https://github.com/insilmaril/vym/blob/release/README.md)
for details

### Features

* Feature: Dark theme
* Feature: Increase max. number of recent maps to 20
* Feature: Reset priority delta for visible tasks (all maps)
* Feature: Toggle target for multiple selected items
* Feature: Copying and pasting between vym instances and pasting images
* Feature: Added desktop files for easier packaging accross Linux distros
* Feature: Larger font size for editing headings on WIndows
* Feature: Scripting commands to edit heading and get depth of branch
* Feature: Introduced colors toolbar (#39)
* Feature: Use expand macro in Confluence export for scrolled branches
* Feature: Move branches diagonally with Ctrl-PageUp/Down
* Feature: Enable openSSL on Windows
* Feature: Add information from JIRA as attributes
* Feature: Toggle flag for multiselection
* Feature: Confluence and JIRA support
* Feature: Cycle tasks by clicking status in taskeditor
* Feature: Cursor up/down + Shift-key can be used to select multiple branches
* Feature: Updated translations for Greek and German

### Bugfixes

* Bugfix: #52 Saving part of map overwrites original map
* Bugfix: #48 lockfile cannot be renamed on Windows
* Bugfix: Read map attributes for default map
* Bugfix: Create translation files during build
* Bugfix: German translation to show keyboard macros in help menu
* Bugfix: Set URL when getting Jira data
* Bugfix: Use mapname and correct postfic when exporting
* Bugfix: #25 treeEditor opens when pasting images
* Bugfix: less compiler warnings related to deprecated Qt
* Bugfix: Restore state of treeEditor and slideEditor from settings in map after load
* Bugfix: Don't set URL for Jira ticket, if Jira pattern is not known
* Bugfix: Umlauts when exporting to a Confluence page
* Bugfix: undo/redo when toggling task via F12-macro
* Bugfix: Allow selecting text while editing a heading in QLineEdit
* Bugfix: Remove invalid QModelIndex warning when relinking images from mainbranch to center
* Bugfix: Avoid jumping of view when adding branches to center"
* Bugfix: Reset current text format when switching from RichText to PlainText
* Bugfix: Unused duplicate branchPropertyEditor dockwidget removed
* Bugfix: Setting  for Windows data-root directory (#36)
* Bugfix: vymBaseDir improvements (#34)
* Bugfix: Don't trigger reposition when selection changes
* Bugfix: Relink branches and keep parent
* Bugfix: set CMAKE_INSTALL_DATAROOTDIR (#24)
* Bugfix: #31 Confluence export missing siblinigs of hidden first branch
* Bugfix: #26 tabname for save but unchanged maps does not update
* Bugfix: Getting user info from Confluence
* Bugfix: Freemind import
* Bugfix: exportLast of Markdown export
* Bugfix: piping plaintext mails from mutt into note
* Bugfix: Cycling tasks in taskeditor
* Bugfix: Links to images (color and hiding)
* Bugfix: Don't give up on unknown tags when importing Freeplane
* Bugfix: Remove unnecessary columns from taskeditor
* Bugfix: #14 Packaging for openSUSE: Set vymBaseDir correctly
* Bugfix: Trash button in NoteEditor
* Bugfix: vym crashed, when cursor left/right was used and multiple branches were selected
* Bugfix: Copy to new map
* Bugfix: crash while checking an empty directory (#9)

### Changes

* Changed: Settings for JIRA and Confluence
* Changed: Moved functionality of recover session into restore ression
* Changed: Sleeping tasks keep their priority (Before prio was lowered and sleeping tasks dropped to bottom.)
* Changed: Removed unused Bugzilla script
* Changed: Builds now use cmake
