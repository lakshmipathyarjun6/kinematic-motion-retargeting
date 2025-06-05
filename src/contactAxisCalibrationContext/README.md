# Retargeting Contacts to Different Hands

This Edit Context plugin enables creating and editing landmark placements on both hands in order to tweak the corresponding atlases. For convenience, it uses axial curves to treat each group of landmarks as a single entity using techniques in <a href="https://dl.acm.org/doi/10.1145/3592117">this paper</a>.

## Setup

If you only intend to use our target hands, you can skip this plugin altogether and instead just download one of our existing annotated files <TODO: link>. 

NOTE: At this time, source code is not available for this plugin. Because it uses many techniques from the cited paper, it was easier to just use a pre-built plugin from source code built for that repository rather than making one from scratch. But we have not yet figured out licensing for that code, and we cannot distribute it without one. Instead, we have created pre-compiled binaries of this plugin for all major operating systems that can be downloaded <TODO: link>here.

To get started, download the binary for your operating system and install it in the same build folder as the others. Then open Maya and set this plugin to auto-load on startup. The plugin assumes that the source hand is labeled as "hand" in the scene outline - the target hand can have any name. 

Next, copy the MEL files under the /scripts subdirectory to the following location:

Mac:
```
cp path/to/project/kinematic-motion-retargeting/src/contactAxisCalibrationContext/scripts/* ~/Library/Preferences/Autodesk/maya/<year>/scripts
```

Linux:
```
cp path/to/project/kinematic-motion-retargeting/src/contactAxisCalibrationContext/scripts/* ~/maya/<year>/scripts
```

Windows:
```
TODO
```

## Plugin activation

Run the following MEL command to activate the plugin:
```
setToolTo acc
```

If successfull, you should see the following Tool Editor menu:

<TODO: TW Image>

## Plugin tools

<TODO: Tool explanations>

## General workflow

1. Import the source hand contacts for the motion you wish to retarget via the <TODO: contactexport> plugin

2. Please watch <TODO: upload and link>this video for a detailed introduction of how to use this plugin

Next step: <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactTransferEditContext">contactTransferEditContext</a>
