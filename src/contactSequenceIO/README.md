# Importing and Exporting Contact Time Series Data

This utility I/O plugin can be used to import and export contacts from one Maya scene to another. It's generally more useful than simply copy-pasting becasue it can store some additional metadata (e.g. omission indices) that can be utilized to ensure that scene parameters are set up correctly. Contacts are exported in JSON format.

## Usage

1. Select the hand (or object) mesh in the viewport or scene outliner whose contacts you want to export.

2. Open the "Import" or "Export Selection" menu under File and change the filetype to "json".

3. Hit the import / export button. If import was successful, contacts should show up as new entities in the scene outliner. If export was successful, a fully populated JSON file should be generated at the filepath specified.

NOTE: Retargeted contacts for all of our target hands and motions can be downloaded from <a href="https://drive.google.com/drive/folders/1cm4nhnLJvYs1p_sssrYCZD2yINgBIE_b">here</a>.
