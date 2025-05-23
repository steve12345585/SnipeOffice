# Online Update

Online update implementation based on Mozilla's MAR format + update mechanism

The source code has been extracted from <https://github.com/mozilla/gecko-dev> with
external/onlineupdate/generate-sources.sh.

The source/service directory contains the code for the silent windows updater that avoids the
repeated administrator check for an update.

## Note

The updater executable should not depend on any other dynamic library in the SnipeOffice
installation as we would need to copy that one also to a temporary directory during update. We can't
update any library or executable that is currently in use. For the updater executable we solve this
problem by copying the updater before using it to a temporary directory.

On Windows we use the system to provide us with a crypto library whereas on Linux we use NSS.

## Update Procedure

The updater executable is run two times. In a first run, the current installation is copied to an
`update` directory and the update is applied in this `update` directory. During the next run, a
replacement request is executed. The replacement request removes the old installation directory and
replaces it with the content of the `update` directory.

### User Profile in the Installation Directory

The archive based installations have the user profile by default inside of the installation
directory. During the update process this causes some problems that need special handling in the
updater.

* The `update` directory is inside of the user profile resulting in recursive copying.
* During the replacement request the updater log is in the user profile, which changes location from
the actual location to a backup location.

## Executable_test_updater_dialog

To run that manual test, do
```
$ cp instdir/program/updater.ini workdir/LinkTarget/Executable/test_updater_dialog.ini
$ workdir/LinkTarget/Executable/test_updater_dialog
$ rm workdir/LinkTarget/Executable/test_updater_dialog.ini
```
