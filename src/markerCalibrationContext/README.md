# Setting Target Hand Virtual Markers

This Edit Context plugin enables creating virtual markers on the target hand.

## Setup

NOTE: Because the received artist configuration used a point landmark set for the source MANO hand, we have hard-coded the locations into the plugin startup routine. If you want to use a different source hand, you will need to modify the code to accommodate the new source hand indices. In the unlikely event there is enough demand to make this routine more agnostic to the source hand, I will consider revising it.

The plugin assumes that the source hand is labeled as "hand" in the scene outline - the target hand can have any name. I would also strongly suggest that both the source and target hand be kept as un-deformed meshes. In other words, consider un-pairing and deleting any rig-based controls. You will otherwise risk running into unexpected behavior when buidling the contact atlases. Please see our provided Maya files for examples <TODO: link>. If you only intend to use our target hands, you can skip this plugin altogether.

## Plugin activation

Run the following MEL command to activate the plugin:
```
setToolTo mcc
```

If successfull, you should see the following Tool Editor menu:

<TODO: TW Image>

## General workflow

Please watch <TODO: upload and link>this video for a detailed introduction of how to use this plugin

Next step: <a href="https://github.com/lakshmipathyarjun6/kinematic-motion-retargeting/tree/main/src/contactAxisCalibrationContext">contactAxisCalibrationContext</a>
