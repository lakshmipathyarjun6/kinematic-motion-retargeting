# Retargeting Contacts to Different Hands

This Edit Context plugin retargets / transfers contacts from one hand to another.

## Setup

This plugin assumes that axis landmarks have already been placed and that the transfer coefficients have already been set. If you are using a new hand and have not yet done so, please use the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactAxisCalibrationContext">contactAxisCalibrationContext</a> plugin first. The plugin assumes that the source hand is labeled as "hand" in the scene outline - the target hand can have any name. 

To get started, first copy the MEL files under the /scripts subdirectory to the following location:

Mac:
```
cp path/to/project/kinematic-motion-retargeting/src/contactTransferEditContext/scripts/* ~/Library/Preferences/Autodesk/maya/<year>/scripts
```

Linux:
```
cp path/to/project/kinematic-motion-retargeting/src/contactTransferEditContext/scripts/* ~/maya/<year>/scripts
```

Windows:
```
TODO
```

## Plugin activation

Run the following MEL command to activate the plugin:
```
setToolTo ctec
```

If successfull, you should see the following Tool Editor menu:

<TODO: TW Image>

## Plugin tools

<TODO: Tool explanations>

## General workflow

1. If you skipped the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactAxisCalibrationContext">contactAxisCalibrationContext</a> plugin, import the source hand contacts for the motion you wish to retarget via the <TODO: contactexport> plugin

2. Retarget contacts for all frames

3. Scan a few frames in the result to check for quality

4. If there are contacts to ignore (e.g. target hand has fewer fingers), set the distance threshold and <TODO: fill in>

5. Export the target hand contacts via the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactSequenceIO">contactSequenceIO</a> plugin

WARNING: This process will take some time, especially if there are a lot of contacts to retarget. It can take anywhere from from a few minutes to overnight to run.

Next step: <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/fusedMotionEditContext">fusedMotionEditContext</a>
