Release notes VYM - View Your Mind
==================================


The lists below shows main changes between the current 3.1 version of vym and
the previous official release 3.0

Main new features in 3.1
------------------------
 * Clone branches
    - The clone inherits most of the properties of the original branch,
      including flags and background color, but no notes or scripts.
      The clone is inserted as a child of the original branch, both
      clone and original have a "chain" flag set and are connected by an
      xlink (dashed line). 
    - Only tasks from original branch are shown in task editor
    - The clone can be moved to another position in the
      map, but the connection to the original branch is maintained. The
      connection can be removed by deleting one end of the xlink or the
      xlink itself.

 * Execute external commands in scripts
    - New class CommandAgent
    - Command: execute in VymModelWrapper
    - Background processes currently not supported
 
 * Scripts can be added to a branch similar to a note. A new system flag
   marks branches with a script: clicking the flag runs the script,
   Shift-clicking opens it in the new "Branch" tab of the script editor.
   There the script can be edited, run and deleted; all changes are part
   of the undo history. New scripting commands: `setScript`,
   `getScript`, `hasScript` and `runScript`

 * Get title from Youtube Url using oembed   


Feel free to report any bugs or feature requests on
[https://github.com/insilmaril/vym/issues](https://github.com/insilmaril/vym/issues)

Thanks for using vym!

Uwe Drechsel - July 2026

## Version 3.0.502
### Features
 * [#221](https://github.com/insilmaril/vym/issues/221) The dialog
   showing texts like the keyboard shortcuts or the available scripting
   commands can be searched now: Entering text and pressing "Search" or
   Return shows only the lines containing the text, "Clear" shows the
   full text again
 * [#222](https://github.com/insilmaril/vym/issues/222) The exports in
   the File menu are sorted alphabetically now. The text based exports
   are collected in a new submenu "Text", where "Text (ASCII)" is called
   "Plaintext" and "Text with tasks" is called "Plaintext with tasks"
   now. The exports for project planning, OrgMode and Taskjuggler, are
   collected in a new submenu "Projects"
 * [#220](https://github.com/insilmaril/vym/issues/220) Clicking a
   system flag of a branch which is not selected yet no longer just
   selects the branch, but also triggers the related action, e.g.
   opening an URL or running a script. Clicking the flag of a scrolled
   branch now unscrolls it
 * [#214](https://github.com/insilmaril/vym/issues/214) Scripts can be
   embedded into a branch, similar to a note. A new system flag marks
   branches with a script: clicking the flag runs the script,
   Shift-clicking opens it in the new "Branch" tab of the script editor.
   There the script can be edited, run and deleted; all changes are part
   of the undo history. New scripting commands: `setScript`,
   `getScript`, `hasScript` and `runScript`
 * [#219](https://github.com/insilmaril/vym/issues/219) New action in
   the Format menu of the TextEditors to remove the background colors of
   the selected text, e.g. to clean up text pasted from web pages
 * [#157](https://github.com/insilmaril/vym/issues/157) New dialog to
   set the parameters of the selection box: border width, border color
   and background color. Both colors can be set including their opacity

### Bugfixes
 * When moving branches to a target, the view no longer scrolls to the
   destination and back, but stays at the branch which is selected
   afterwards near the original position. A scrolled destination is no
   longer temporary unscrolled while moving
 * Clicking the system flag "Hide object in exported maps" toggles the
   setting again, the flag name was compared without its "system-"
   prefix before
 * Selecting the map background color no longer creates an undo step for
   every color touched while the color dialog is open, but only one for
   the finally accepted color. Cancelling now also restores an existing
   background image, which previously was removed by the preview

