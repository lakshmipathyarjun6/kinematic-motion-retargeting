# B-Spline Fitting of Retargetd Motion

This Edit Context plugin is the final one in the processing sequence, which fits the final estimated motion trajectory with B-Splines to compute the final result.

## Setup

This plugin assumes a scene that has been fully processed using the <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/fusedMotionEditContext">fusedMotionEditContext</a> plugin.

To get started, first copy the MEL files under the /scripts subdirectory to the following location:

Mac:
```
cp path/to/project/kinematic-motion-retargeting/src/smoothMotionEditContext/scripts/* ~/Library/Preferences/Autodesk/maya/<year>/scripts
```

Linux:
```
cp path/to/project/kinematic-motion-retargeting/src/smoothMotionEditContext/scripts/* ~/maya/<year>/scripts
```

Windows:
```
TODO
```

## Plugin activation

Run the following MEL command to activate the plugin:
```
setToolTo smec
```

If successfull, you should see the following Tool Editor menu:

<TODO: TW Image>

## Plugin tools

<TODO: Tool explanations>

## General workflow

1. Compute fitted B-Splines for all hand DOFs across all frames

2. Select the generated splines in the scene outline, then hit (TODO: specifics) in the tool plugin window to load the spline data into the actual hand DOFs and store the resulting keyframes

3. Your final retarted result is complete! Enjoy!

[TODO: Exports??]
