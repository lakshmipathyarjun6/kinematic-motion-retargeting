# Importing and Exporting Contact Virtual Marker Data

This utility I/O plugin can be used to import and export virtual markers from one Maya scene to another. Markers are stored in JSON format, although the extension of "virtualmarkers" is used to prevent extension conflicts with the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactSequenceIO">contactSequenceIO</a> plugin (Maya only allows one extension per I/O plugin).

## Usage

1. Select the hand mesh in the viewport or scene outliner whose contacts you want to export.

2. Open the "Import" or "Export Selection" menu under File and change the filetype to "virtualmarkers".

3. Hit the import / export button. If import was successful, virtual markers should show up as new entities in the scene outliner. If export was successful, a fully populated JSON file (but using the virtualmarkers extension) should be generated at the filepath specified.
